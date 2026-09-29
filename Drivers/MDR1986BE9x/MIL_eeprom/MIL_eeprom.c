/**
 * @file MIL_eeprom.c
 * @brief Flash/EEPROM memory controller library for Milandr 1986VE91T
 * @author Mistress-Lukutar
 * @date 2026-09-29
 * @version v1.2.4
 *
 * Implementation of Flash memory operations following the timing diagrams
 * and specifications from Milandr datasheet ТСКЯ.431296.001СП.
 */

#include "MIL_eeprom.h"

/* ============================================================================
 * Private Defines
 * ============================================================================
 */

/** @defgroup MIL_EEPROM_Private_Defines Internal Constants
 * @{
 */
#define EEPROM_ERASED_VALUE 0xFFFFFFFFUL    /**< Value after erase */
#define EEPROM_ADDR_ALIGN_MASK 0x00000003UL /**< 4-byte alignment */
#define EEPROM_PAGE_ADDR_MASK 0x0001F000UL  /**< Page address bits */
#define EEPROM_PAGE_ADDR_SHIFT 12UL         /**< Page address shift */
#define EEPROM_GUARD_US 1UL                 /**< 1us guard covering the txa/tpgh sub-us minimums (30/20ns) */
#define EEPROM_READ_SETTLE_US 50UL          /**< Flash read-path settle after CON drops (empirical, not a datasheet value) */
#define EEPROM_RESYNC_READS 16UL            /**< Dummy flash reads that walk the read FSM before flash code executes again */
/** @} */

/* ============================================================================
 * Private Function Prototypes
 * ============================================================================
 */

__RAMFUNC static void _delayUs(uint32_t us);
__RAMFUNC static void _delayMs(uint32_t ms);
__RAMFUNC static void _enterProgrammingMode(void);
__RAMFUNC static void _exitProgrammingMode(void);
__RAMFUNC
static void _eraseSector(uint32_t address, uint32_t sector, MIL_EEPROM_MemType memType, uint8_t isMassErase);

/* ============================================================================
 * Private Functions - Timing
 * ============================================================================
 */

/**
 * @brief Busy-wait delay in microseconds, based on the SysTick hardware counter
 * @param us Delay time in microseconds
 * @note Polls the free-running SysTick down-counter.
 * @note Falls back to a conservative NOP loop if SysTick is not running yet
 *       (early boot); the fallback may over-delay but never under-delays.
 * @note RAM-resident: runs with CON=1 while flash is not readable.
 */
__RAMFUNC static void _delayUs(uint32_t us) {
  if (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) {
    const uint32_t target_cycles = us * SYSTEM_CORE_CLOCK_MHZ;
    const uint32_t reload        = SysTick->LOAD + 1U;
    uint32_t prev                = SysTick->VAL;
    uint32_t elapsed             = 0U;

    while (elapsed < target_cycles) {
      const uint32_t now = SysTick->VAL;
      /* VAL counts down; on wrap the remaining span is prev + (reload - now) */
      elapsed += (now <= prev) ? (prev - now) : (prev + reload - now);
      prev = now;
    }
  } else {
    /* SysTick not started: NOP loop. The volatile counter forces a load/store
     * per iteration, so the real cost is >= the assumed 4 cycles even at -O3. */
    volatile uint32_t count = us * (SYSTEM_CORE_CLOCK_MHZ / 4UL);
    if (count == 0U) {
      count = 1U;
    }
    while (count > 0U) {
      count--;
      __NOP();
    }
  }
}

/**
 * @brief Busy-wait delay in milliseconds
 * @param ms Delay time in milliseconds
 */
__RAMFUNC static void _delayMs(uint32_t ms) {
  while (ms--) {
    _delayUs(1000);
  }
}

/* ============================================================================
 * Private Functions - Mode Control
 * ============================================================================
 */

/**
 * @brief Enter programming mode
 * @note Must be called from RAM when performing flash operations
 */
__RAMFUNC static void _enterProgrammingMode(void) {
  /* Retire in-flight flash accesses before the array goes away */
  __DSB();
  __ISB();

  /* Write unlock key */
  MDR_EEPROM->KEY = EEPROM_KEY;

  /* Set CON bit to enter programming mode */
  MDR_EEPROM->CMD |= EEPROM_CMD_CON;

  /* Drain the write buffer so CON is latched before any timed sequence */
  __DSB();
}

/**
 * @brief Exit programming mode and return to normal operation
 */
__RAMFUNC static void _exitProgrammingMode(void) {
  /* Clear all control bits */
  MDR_EEPROM->CMD &= ~(EEPROM_CMD_CON | EEPROM_CMD_XE | EEPROM_CMD_YE | EEPROM_CMD_SE | EEPROM_CMD_NVSTR | EEPROM_CMD_PROG | EEPROM_CMD_ERASE
      | EEPROM_CMD_MAS1 | EEPROM_CMD_IFREN);

  /* Ensure the final command write has reached the peripheral before the
   * key is cleared and interrupts are restored */
  __DSB();

  /* Clear the key */
  MDR_EEPROM->KEY = 0;

  /* The flash read path stays desynchronized for a while after CON drops
   * and serves stale-line data to instruction fetches (seen as garbage
   * execution). Settle, then walk the read FSM with dummy line reads
   * before any flash-resident code runs again. */
  _delayUs(EEPROM_READ_SETTLE_US);
  {
    volatile uint32_t* resync = (volatile uint32_t*)0x08000000UL; /* 32 bytes apart: distinct flash lines */
    uint32_t n;
    for (n = 0U; n < EEPROM_RESYNC_READS; n++) {
      (void)*resync;
      __DSB();
      resync += 8U;
    }
  }

  /* Final settle after the last dummy read: ending the exit path right
   * after the resync loop let the very next instruction fetch - the
   * perpetually pending 153 kHz PWM vector fetch - catch the read FSM
   * mid-recovery and enter garbage code (undefined-instruction HardFault
   * with a mid-instruction PC inside the target ISR). */
  _delayUs(EEPROM_READ_SETTLE_US);
}

/**
 * @brief Erase a single sector of memory
 * @param address Base address for the operation
 * @param sector Sector number (EEPROM_SECTOR_A/B/C/D)
 * @param memType Memory type selection
 * @param isMassErase 1 for mass erase, 0 for page erase
 */
__RAMFUNC static void _eraseSector(uint32_t address, uint32_t sector, MIL_EEPROM_MemType memType, uint8_t isMassErase) {
  uint32_t cmd = MDR_EEPROM->CMD;

  /* Set address with sector bits */
  MDR_EEPROM->ADR = (address & ~EEPROM_SECTOR_MASK) | sector;

  /* Clear data input, as the vendor SPL does before every erase pulse */
  MDR_EEPROM->DI = 0;

  /* Configure memory type */
  if (memType == EEPROM_INFO_MEMORY) {
    cmd |= EEPROM_CMD_IFREN;
  } else {
    cmd &= ~EEPROM_CMD_IFREN;
  }

  /* Set XE and ERASE bits */
  cmd |= (EEPROM_CMD_XE | EEPROM_CMD_ERASE);

  /* For mass erase, also set MAS1 bit */
  if (isMassErase) {
    cmd |= EEPROM_CMD_MAS1;
  } else {
    cmd &= ~EEPROM_CMD_MAS1;
  }

  MDR_EEPROM->CMD = cmd;
  /* Commit before timing tnvs from this point */
  __DSB();

  /* Wait tnvs = 5us */
  _delayUs(EEPROM_TIME_TNVS_US);

  /* Set NVSTR bit */
  MDR_EEPROM->CMD |= EEPROM_CMD_NVSTR;

  /* Wait for erase: tme = 40ms for mass, terase = 40ms for page */
  if (isMassErase) {
    _delayMs(EEPROM_TIME_TME_MS);
  } else {
    _delayMs(EEPROM_TIME_TERASE_MS);
  }

  /* Clear ERASE bit */
  MDR_EEPROM->CMD &= ~EEPROM_CMD_ERASE;

  /* Wait tnvh1 = 100us for mass erase, tnvh = 5us for page erase */
  if (isMassErase) {
    _delayUs(EEPROM_TIME_TNVH1_US);
  } else {
    _delayUs(EEPROM_TIME_TNVH_US);
  }

  /* Clear XE, MAS1, NVSTR bits */
  MDR_EEPROM->CMD &= ~(EEPROM_CMD_XE | EEPROM_CMD_MAS1 | EEPROM_CMD_NVSTR);

  /* Wait trcv = 1us recovery time */
  _delayUs(EEPROM_TIME_TRCV_US);
}

/* ============================================================================
 * Public Functions - Initialization
 * ============================================================================
 */

/**
 * @brief Initialize the EEPROM controller
 * @param config Pointer to configuration structure
 * @return EEPROM_OK on success, EEPROM_ERROR_INVALID_CONFIG on bad config
 */
MIL_EEPROM_Status MIL_EEPROM_Init(const MIL_EEPROM_Config* config) {
  if (config == NULL) {
    return EEPROM_ERROR_INVALID_CONFIG;
  }

  /* Cross-check the flash access delay against the declared CPU frequency */
  if (config->delayValue != MIL_EEPROM_CalculateDelay(config->cpuFreqHz)) {
    return EEPROM_ERROR_INVALID_CONFIG;
  }

  /* Enable EEPROM controller clock */
  MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_EEPROM_CNTRL;

  /* Set flash access delay */
  MIL_EEPROM_SetDelay(config->delayValue);

  return EEPROM_OK;
}

/**
 * @brief Deinitialize the EEPROM controller
 */
void MIL_EEPROM_DeInit(void) {
  /* Ensure we're in normal mode */
  _exitProgrammingMode();

  /* Optionally disable clock to save power */
  /* MDR_RST_CLK->PER_CLOCK &= ~RST_CLK_PER_CLOCK_PCLK_EN_EEPROM_CNTRL; */
}

/**
 * @brief Set flash access delay based on CPU frequency
 * @param delayValue Delay value (0-7)
 */
void MIL_EEPROM_SetDelay(uint8_t delayValue) {
  uint32_t cmd = MDR_EEPROM->CMD;

  /* Clear existing delay bits and set new value */
  cmd &= ~EEPROM_CMD_DELAY_Msk;
  cmd |= ((uint32_t)(delayValue & 0x07) << EEPROM_CMD_DELAY_Pos);

  MDR_EEPROM->CMD = cmd;
}

/**
 * @brief Calculate appropriate delay value for given frequency
 * @param freqHz CPU frequency in Hz
 * @return Appropriate delay value (0-7)
 */
uint8_t MIL_EEPROM_CalculateDelay(uint32_t freqHz) {
  if (freqHz <= 25000000UL)
    return EEPROM_DELAY_0_CYCLES;
  if (freqHz <= 50000000UL)
    return EEPROM_DELAY_1_CYCLE;
  if (freqHz <= 75000000UL)
    return EEPROM_DELAY_2_CYCLES;
  if (freqHz <= 100000000UL)
    return EEPROM_DELAY_3_CYCLES;
  if (freqHz <= 125000000UL)
    return EEPROM_DELAY_4_CYCLES;
  if (freqHz <= 150000000UL)
    return EEPROM_DELAY_5_CYCLES;
  if (freqHz <= 175000000UL)
    return EEPROM_DELAY_6_CYCLES;
  return EEPROM_DELAY_7_CYCLES;
}

/* ============================================================================
 * Public Functions - Read Operations
 * ============================================================================
 */

/**
 * @brief Read a 32-bit word from flash memory (normal mode)
 * @param address Memory address (must be 4-byte aligned)
 * @return 32-bit data read from memory
 */
uint32_t MIL_EEPROM_ReadWord(uint32_t address) {
  /* Direct memory access in normal mode */
  return *((volatile uint32_t*)address);
}

/**
 * @brief Read a 32-bit word in programming mode
 * @param address Memory address (must be 4-byte aligned)
 * @param memType Memory type (main or information)
 * @return 32-bit data read from memory
 */
__RAMFUNC uint32_t MIL_EEPROM_ReadWordProg(uint32_t address, MIL_EEPROM_MemType memType) {
  uint32_t data;
  uint32_t cmd;
  uint32_t primask;
  primask = __get_PRIMASK();
  __disable_irq();

  _enterProgrammingMode();

  /* Set address */
  MDR_EEPROM->ADR = address;

  /* Configure command: set IFREN for info memory */
  cmd = MDR_EEPROM->CMD;
  cmd &= ~EEPROM_CMD_IFREN;
  if (memType == EEPROM_INFO_MEMORY) {
    cmd |= EEPROM_CMD_IFREN;
  }

  /* Set XE, YE, SE bits for read operation */
  cmd |= (EEPROM_CMD_XE | EEPROM_CMD_YE | EEPROM_CMD_SE);
  MDR_EEPROM->CMD = cmd;
  /* Commit before timing the txa access guard */
  __DSB();

  /* Wait for data to be valid */
  _delayUs(EEPROM_GUARD_US);

  /* Read data from DO register */
  data = MDR_EEPROM->DO;

  /* Clear control bits */
  MDR_EEPROM->CMD &= ~(EEPROM_CMD_XE | EEPROM_CMD_YE | EEPROM_CMD_SE);

  /* Read recovery time before leaving programming mode */
  _delayUs(EEPROM_TIME_TRCV_US);

  _exitProgrammingMode();

  if (!primask) {
    __enable_irq();
  }
  return data;
}

/**
 * @brief Read multiple words from flash memory
 * @param address Start address (must be 4-byte aligned)
 * @param buffer Pointer to destination buffer
 * @param wordCount Number of 32-bit words to read
 * @param memType Memory type (main or information)
 * @return EEPROM_OK on success
 */
__RAMFUNC MIL_EEPROM_Status MIL_EEPROM_ReadBuffer(uint32_t address, uint32_t* buffer, uint32_t wordCount, MIL_EEPROM_MemType memType) {
  uint32_t i;
  uint32_t cmd;
  uint32_t primask;

  /* Validate parameters */
  if (buffer == NULL) {
    return EEPROM_ERROR_INVALID_ADDR;
  }
  if ((address & EEPROM_ADDR_ALIGN_MASK) != 0) {
    return EEPROM_ERROR_ALIGNMENT;
  }

  primask = __get_PRIMASK();
  __disable_irq();

  _enterProgrammingMode();

  /* Configure command: set IFREN for info memory */
  cmd = MDR_EEPROM->CMD;
  cmd &= ~EEPROM_CMD_IFREN;
  if (memType == EEPROM_INFO_MEMORY) {
    cmd |= EEPROM_CMD_IFREN;
  }

  /* One full command pulse per word (the Milandr SPL EEPROM_ReadWord
   * pattern): ADR must only change while XE/YE/SE are low. */
  for (i = 0; i < wordCount; i++) {
    /* Set address with the command bits quiescent */
    MDR_EEPROM->ADR = address + (i * 4);

    /* Strobe the read command */
    MDR_EEPROM->CMD = cmd | EEPROM_CMD_XE | EEPROM_CMD_YE | EEPROM_CMD_SE;
    __DSB();

    /* Idle DO reads for address-to-data settling, as in the vendor SPL */
    (void)MDR_EEPROM->DO;
    (void)MDR_EEPROM->DO;
    (void)MDR_EEPROM->DO;

    /* Read data */
    buffer[i] = MDR_EEPROM->DO;

    /* Drop the command before the next address change */
    MDR_EEPROM->CMD = cmd;
  }

  /* Read recovery time before leaving programming mode */
  _delayUs(EEPROM_TIME_TRCV_US);

  _exitProgrammingMode();

  if (!primask) {
    __enable_irq();
  }
  return EEPROM_OK;
}

/* ============================================================================
 * Public Functions - Write Operations
 * ============================================================================
 */

/**
 * @brief Write a 32-bit word to flash memory
 * @param address Memory address (must be 4-byte aligned)
 * @param data 32-bit data to write
 * @param memType Memory type (main or information)
 * @return EEPROM_OK on success
 */
__RAMFUNC MIL_EEPROM_Status MIL_EEPROM_WriteWord(uint32_t address, uint32_t data, MIL_EEPROM_MemType memType) {
  uint32_t cmd;
  uint32_t primask;

  /* Validate alignment */
  if ((address & EEPROM_ADDR_ALIGN_MASK) != 0) {
    return EEPROM_ERROR_ALIGNMENT;
  }

  primask = __get_PRIMASK();
  __disable_irq();

  _enterProgrammingMode();

  /* Set address */
  MDR_EEPROM->ADR = address;

  /* Set data to write */
  MDR_EEPROM->DI = data;

  /* Configure command: set IFREN for info memory */
  cmd = MDR_EEPROM->CMD;
  cmd &= ~EEPROM_CMD_IFREN;
  if (memType == EEPROM_INFO_MEMORY) {
    cmd |= EEPROM_CMD_IFREN;
  }

  /* Set XE and PROG bits */
  cmd |= (EEPROM_CMD_XE | EEPROM_CMD_PROG);
  MDR_EEPROM->CMD = cmd;
  /* Commit before timing tnvs from this point */
  __DSB();

  /* Wait tnvs = 5us */
  _delayUs(EEPROM_TIME_TNVS_US);

  /* Set NVSTR bit */
  MDR_EEPROM->CMD |= EEPROM_CMD_NVSTR;

  /* Wait tpgs = 10us */
  _delayUs(EEPROM_TIME_TPGS_US);

  /* Set YE bit */
  MDR_EEPROM->CMD |= EEPROM_CMD_YE;

  /* Wait tprog = 40us for programming */
  _delayUs(EEPROM_TIME_TPROG_US);

  /* Clear YE bit */
  MDR_EEPROM->CMD &= ~EEPROM_CMD_YE;

  /* Wait tpgh = 20ns minimum, covered by a 1us hardware-timed guard */
  _delayUs(EEPROM_GUARD_US);

  /* Clear PROG bit */
  MDR_EEPROM->CMD &= ~EEPROM_CMD_PROG;

  /* Wait tnvh = 5us */
  _delayUs(EEPROM_TIME_TNVH_US);

  /* Clear XE and NVSTR bits */
  MDR_EEPROM->CMD &= ~(EEPROM_CMD_XE | EEPROM_CMD_NVSTR);

  /* Wait trcv = 1us recovery time */
  _delayUs(EEPROM_TIME_TRCV_US);

  _exitProgrammingMode();

  if (!primask) {
    __enable_irq();
  }
  return EEPROM_OK;
}

/**
 * @brief Write multiple words to flash memory
 * @param address Start address (must be 4-byte aligned)
 * @param buffer Pointer to source data buffer
 * @param wordCount Number of 32-bit words to write
 * @param memType Memory type (main or information)
 * @return EEPROM_OK on success
 */
__RAMFUNC MIL_EEPROM_Status MIL_EEPROM_WriteBuffer(uint32_t address, const uint32_t* buffer, uint32_t wordCount, MIL_EEPROM_MemType memType) {
  uint32_t i;
  for (i = 0; i < wordCount; i++) {
    MIL_EEPROM_Status status = MIL_EEPROM_WriteWord(address + (i * 4), buffer[i], memType);
    if (status != EEPROM_OK)
      return status;
  }
  return EEPROM_OK;
}

/* ============================================================================
 * Public Functions - Erase Operations
 * ============================================================================
 */

/**
 * @brief Erase a single 4KB page
 * @param pageAddress Any address within the page to erase
 * @param memType Memory type (main or information)
 * @return EEPROM_OK on success
 */
__RAMFUNC MIL_EEPROM_Status MIL_EEPROM_ErasePage(uint32_t pageAddress, MIL_EEPROM_MemType memType) {
  uint32_t primask = __get_PRIMASK();
  __disable_irq();

  _enterProgrammingMode();

  /* Erase all four sectors of the page (A, B, C, D) */
  _eraseSector(pageAddress, EEPROM_SECTOR_A, memType, 0);
  _eraseSector(pageAddress, EEPROM_SECTOR_B, memType, 0);
  _eraseSector(pageAddress, EEPROM_SECTOR_C, memType, 0);
  _eraseSector(pageAddress, EEPROM_SECTOR_D, memType, 0);

  _exitProgrammingMode();

  if (!primask) {
    __enable_irq();
  }
  return EEPROM_OK;
}

/**
 * @brief Erase all memory (mass erase)
 * @param memType Memory type (main or information)
 * @return EEPROM_OK on success
 * @warning Erasing info memory also erases main memory!
 * @note Holds the flash in programming mode for 4 x ~40 ms with interrupts
 *       disabled; callers must tolerate the resulting system stall.
 */
__RAMFUNC MIL_EEPROM_Status MIL_EEPROM_EraseAll(MIL_EEPROM_MemType memType) {
  /* Same locking as MIL_EEPROM_ErasePage: with CON=1 any interrupt would
   * fetch code from a non-readable flash */
  uint32_t primask = __get_PRIMASK();
  __disable_irq();

  _enterProgrammingMode();

  /* Erase all four sectors with mass erase */
  _eraseSector(EEPROM_MAIN_BASE_ADDR, EEPROM_SECTOR_A, memType, 1);
  _eraseSector(EEPROM_MAIN_BASE_ADDR, EEPROM_SECTOR_B, memType, 1);
  _eraseSector(EEPROM_MAIN_BASE_ADDR, EEPROM_SECTOR_C, memType, 1);
  _eraseSector(EEPROM_MAIN_BASE_ADDR, EEPROM_SECTOR_D, memType, 1);

  _exitProgrammingMode();

  if (!primask) {
    __enable_irq();
  }
  return EEPROM_OK;
}

/* ============================================================================
 * Public Functions - Utility Functions
 * ============================================================================
 */

/**
 * @brief Verify written data against source buffer
 * @param address Start address to verify
 * @param buffer Expected data buffer
 * @param wordCount Number of words to verify
 * @param memType Memory type (main or information)
 * @return EEPROM_OK if data matches
 */
MIL_EEPROM_Status MIL_EEPROM_Verify(uint32_t address, const uint32_t* buffer, uint32_t wordCount, MIL_EEPROM_MemType memType) {
  uint32_t i;
  uint32_t readData;

  if (buffer == NULL) {
    return EEPROM_ERROR_INVALID_ADDR;
  }

  for (i = 0; i < wordCount; i++) {
    readData = MIL_EEPROM_ReadWordProg(address + (i * 4), memType);
    if (readData != buffer[i]) {
      return EEPROM_ERROR_VERIFY;
    }
  }

  return EEPROM_OK;
}

/**
 * @brief Check if address range is erased (all 0xFFFFFFFF)
 * @param address Start address
 * @param wordCount Number of words to check
 * @param memType Memory type
 * @return EEPROM_OK if erased, EEPROM_ERROR_VERIFY if not
 */
MIL_EEPROM_Status MIL_EEPROM_IsErased(uint32_t address, uint32_t wordCount, MIL_EEPROM_MemType memType) {
  uint32_t i;
  uint32_t readData;

  for (i = 0; i < wordCount; i++) {
    readData = MIL_EEPROM_ReadWordProg(address + (i * 4), memType);
    if (readData != EEPROM_ERASED_VALUE) {
      return EEPROM_ERROR_VERIFY;
    }
  }

  return EEPROM_OK;
}

/**
 * @brief Get page number from address
 * @param address Memory address
 * @return Page number (0-31 for main memory)
 */
uint32_t MIL_EEPROM_GetPageNumber(uint32_t address) { return ((address - EEPROM_MAIN_BASE_ADDR) >> EEPROM_PAGE_ADDR_SHIFT); }

/**
 * @brief Get page start address
 * @param pageNumber Page number
 * @return Start address of the page
 */
uint32_t MIL_EEPROM_GetPageAddress(uint32_t pageNumber) { return (EEPROM_MAIN_BASE_ADDR + (pageNumber << EEPROM_PAGE_ADDR_SHIFT)); }
