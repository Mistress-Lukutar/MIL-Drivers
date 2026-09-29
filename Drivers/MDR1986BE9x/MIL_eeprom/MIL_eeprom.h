/**
 * @file MIL_eeprom.h
 * @brief Flash/EEPROM memory controller library for Milandr 1986VE91T
 * @author Mistress-Lukutar
 * @date 2026-09-29
 * @version v1.2.4
 *
 * This library provides functions for reading, writing, and erasing
 * the internal Flash memory (main and information blocks) on Milandr
 * 1986VE9x series microcontrollers.
 */

#ifndef MIL_EEPROM_H
#define MIL_EEPROM_H

#include "MDR32Fx.h"
#include <stdint.h>
#include <stdlib.h>

/* Default system clock if not defined externally */
#ifndef SYSTEM_CORE_CLOCK_MHZ
#define SYSTEM_CORE_CLOCK_MHZ 80UL /**< Default core clock frequency (MHz) */
#endif

/* ============================================================================
 * Compiler specific defines for RAM functions
 * ============================================================================
 */
#if defined(__ICCARM__)
#define __RAMFUNC __ramfunc
#elif defined(__CMCARM__)
#define __RAMFUNC __ramfunc
#elif defined(__CC_ARM)
#define __RAMFUNC __attribute__((section(".ramfunc"), used, noinline))
#elif defined(__GNUC__)
#define __RAMFUNC __attribute__((section(".ramfunc"), used, noinline))
#endif

/* ============================================================================
 * Memory Configuration Constants
 * ============================================================================
 */

/** @defgroup EEPROM_Memory_Map Memory Map Configuration
 * @{
 */
#define EEPROM_MAIN_BASE_ADDR 0x08000000UL /**< Main memory start */
#define EEPROM_MAIN_SIZE 0x00020000UL      /**< 128 KB main memory */
#define EEPROM_INFO_SIZE 0x00001000UL      /**< 4 KB info memory */
#define EEPROM_PAGE_SIZE_BYTES 4096UL      /**< Page size (4 KB) */
#define EEPROM_TOTAL_PAGES 32UL            /**< Total pages in main */
/** @} */

/* ============================================================================
 * Timing Constants (based on datasheet)
 * ============================================================================
 */

/** @defgroup EEPROM_Timing Timing Parameters
 * @brief Datasheet minimums (ТСКЯ.431296.001СП). The sub-microsecond values
 *        (TPGH/TADH/TADS/TXA) are reference-only: the driver covers them with
 *        a single 1 us hardware-timed guard delay (EEPROM_GUARD_US).
 * @{
 */
#define EEPROM_TIME_TNVS_US 5UL    /**< XE to NVSTR setup time */
#define EEPROM_TIME_TME_MS 40UL    /**< Mass erase time */
#define EEPROM_TIME_TERASE_MS 40UL /**< Page erase time */
#define EEPROM_TIME_TNVH1_US 100UL /**< NVSTR hold after mass erase */
#define EEPROM_TIME_TNVH_US 5UL    /**< NVSTR hold after page erase */
#define EEPROM_TIME_TRCV_US 1UL    /**< Recovery time */
#define EEPROM_TIME_TPGS_US 10UL   /**< Program setup time */
#define EEPROM_TIME_TPROG_US 40UL  /**< Programming time */
#define EEPROM_TIME_TPGH_NS 20UL   /**< PROG hold time (covered by 1 us guard) */
#define EEPROM_TIME_TADH_NS 20UL   /**< Address hold time (covered by 1 us guard) */
#define EEPROM_TIME_TADS_NS 20UL   /**< Address setup time (covered by 1 us guard) */
#define EEPROM_TIME_TXA_NS 30UL    /**< Read access time (covered by 1 us guard) */
/** @} */

/* ============================================================================
 * Sector Definitions
 * ============================================================================
 */

/** @defgroup EEPROM_Sectors Sector Configuration
 * @{
 */
#define EEPROM_SECTOR_A 0x00UL    /**< Sector A (bits [3:2] = 00) */
#define EEPROM_SECTOR_B 0x04UL    /**< Sector B (bits [3:2] = 01) */
#define EEPROM_SECTOR_C 0x08UL    /**< Sector C (bits [3:2] = 10) */
#define EEPROM_SECTOR_D 0x0CUL    /**< Sector D (bits [3:2] = 11) */
#define EEPROM_SECTOR_MASK 0x0CUL /**< Sector bits mask */
#define EEPROM_SECTOR_COUNT 4UL   /**< Number of sectors */
/** @} */

/* ============================================================================
 * Delay Configuration for Different Clock Frequencies
 * ============================================================================
 */

/** @defgroup EEPROM_Delay Flash Access Delay Settings
 * @brief Delay[2:0] values based on CPU frequency
 * @{
 */
#define EEPROM_DELAY_0_CYCLES 0x00UL /**< Up to 25 MHz */
#define EEPROM_DELAY_1_CYCLE 0x01UL  /**< Up to 50 MHz */
#define EEPROM_DELAY_2_CYCLES 0x02UL /**< Up to 75 MHz */
#define EEPROM_DELAY_3_CYCLES 0x03UL /**< Up to 100 MHz (max 80 MHz) */
#define EEPROM_DELAY_4_CYCLES 0x04UL /**< Up to 125 MHz (default) */
#define EEPROM_DELAY_5_CYCLES 0x05UL /**< Up to 150 MHz */
#define EEPROM_DELAY_6_CYCLES 0x06UL /**< Up to 175 MHz */
#define EEPROM_DELAY_7_CYCLES 0x07UL /**< Up to 200 MHz */
/** @} */

/* ============================================================================
 * Memory Type Selection
 * ============================================================================
 */

/** @defgroup MIL_EEPROM_MemType Memory Type Selection
 * @{
 */
typedef enum {
  EEPROM_MAIN_MEMORY = 0, /**< Main program memory (128 KB) */
  EEPROM_INFO_MEMORY = 1  /**< Information memory block (4 KB) */
} MIL_EEPROM_MemType;
/** @} */

/* ============================================================================
 * Return Status Codes
 * ============================================================================
 */

/** @defgroup MIL_EEPROM_Status Status Codes
 * @{
 */
typedef enum {
  EEPROM_OK = 0,              /**< Operation completed successfully */
  EEPROM_ERROR_INVALID_ADDR,  /**< Invalid address specified */
  EEPROM_ERROR_ALIGNMENT,     /**< Address not 32-bit aligned */
  EEPROM_ERROR_TIMEOUT,       /**< Operation timeout */
  EEPROM_ERROR_VERIFY,        /**< Write verification failed */
  EEPROM_ERROR_INVALID_CONFIG /**< Invalid controller configuration */
} MIL_EEPROM_Status;
/** @} */

/* ============================================================================
 * Configuration Structure
 * ============================================================================
 */

/**
 * @brief EEPROM configuration structure
 */
typedef struct {
  uint32_t cpuFreqHz; /**< CPU frequency in Hz for timing calculation */
  uint8_t delayValue; /**< Flash access delay (0-7 based on frequency) */
} MIL_EEPROM_Config;

/* ============================================================================
 * Function Prototypes - Initialization
 * ============================================================================
 */

/**
 * @brief Initialize the EEPROM controller
 * @param config Pointer to configuration structure
 * @return EEPROM_OK on success
 */
MIL_EEPROM_Status MIL_EEPROM_Init(const MIL_EEPROM_Config* config);

/**
 * @brief Deinitialize the EEPROM controller
 */
void MIL_EEPROM_DeInit(void);

/**
 * @brief Set flash access delay based on CPU frequency
 * @param delayValue Delay value (0-7)
 */
void MIL_EEPROM_SetDelay(uint8_t delayValue);

/**
 * @brief Calculate appropriate delay value for given frequency
 * @param freqHz CPU frequency in Hz
 * @return Appropriate delay value (0-7)
 */
uint8_t MIL_EEPROM_CalculateDelay(uint32_t freqHz);

/* ============================================================================
 * Function Prototypes - Read Operations
 * ============================================================================
 */

/**
 * @brief Read a 32-bit word from flash memory (normal mode)
 * @param address Memory address (must be 4-byte aligned)
 * @return 32-bit data read from memory
 * @note For main memory access in normal operating mode
 */
uint32_t MIL_EEPROM_ReadWord(uint32_t address);

/* ============================================================================
 * Function Prototypes - Utility Functions
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
MIL_EEPROM_Status MIL_EEPROM_Verify(uint32_t address, const uint32_t* buffer, uint32_t wordCount, MIL_EEPROM_MemType memType);

/**
 * @brief Check if address range is erased (all 0xFFFFFFFF)
 * @param address Start address
 * @param wordCount Number of words to check
 * @param memType Memory type
 * @return EEPROM_OK if erased, EEPROM_ERROR_VERIFY if not
 */
MIL_EEPROM_Status MIL_EEPROM_IsErased(uint32_t address, uint32_t wordCount, MIL_EEPROM_MemType memType);

/**
 * @brief Get page number from address
 * @param address Memory address
 * @return Page number (0-31 for main memory)
 */
uint32_t MIL_EEPROM_GetPageNumber(uint32_t address);

/**
 * @brief Get page start address
 * @param pageNumber Page number
 * @return Start address of the page
 */
uint32_t MIL_EEPROM_GetPageAddress(uint32_t pageNumber);

/* ============================================================================
 * Function Prototypes - Write Operations (RAM required)
 * ============================================================================
 */
__RAMFUNC MIL_EEPROM_Status MIL_EEPROM_WriteWord(uint32_t address, uint32_t data, MIL_EEPROM_MemType memType);

__RAMFUNC MIL_EEPROM_Status MIL_EEPROM_WriteBuffer(uint32_t address, const uint32_t* buffer, uint32_t wordCount, MIL_EEPROM_MemType memType);

/* ============================================================================
 * Function Prototypes - Erase Operations (RAM required)
 * ============================================================================
 */
__RAMFUNC MIL_EEPROM_Status MIL_EEPROM_ErasePage(uint32_t pageAddress, MIL_EEPROM_MemType memType);

__RAMFUNC MIL_EEPROM_Status MIL_EEPROM_EraseAll(MIL_EEPROM_MemType memType);

/* ============================================================================
 * Function Prototypes - Programming Mode Read (RAM required)
 * ============================================================================
 */
__RAMFUNC uint32_t MIL_EEPROM_ReadWordProg(uint32_t address, MIL_EEPROM_MemType memType);
__RAMFUNC MIL_EEPROM_Status MIL_EEPROM_ReadBuffer(uint32_t address, uint32_t* buffer, uint32_t wordCount, MIL_EEPROM_MemType memType);

#endif /* MIL_EEPROM_H */
