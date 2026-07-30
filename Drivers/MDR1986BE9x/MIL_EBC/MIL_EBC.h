/**
 * @file   MIL_EBC.h
 * @brief  External Bus Controller (EBC) driver for Milandr MDR1986VE9x
 * @author Mistress-Lukutar
 * @date   2026-05-19
 * @version v1.0.0
 */

#ifndef MIL_EBC_H
#define MIL_EBC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>

#include "MDR32Fx.h"

/* Exported constants --------------------------------------------------------*/

/** @defgroup MIL_EBC_Control EBC Control register bits
 *  @{
 */
#define MIL_EBC_CONTROL_ROM (1U << 0)  /**< ROM mode enable */
#define MIL_EBC_CONTROL_RAM (1U << 1)  /**< RAM mode enable */
#define MIL_EBC_CONTROL_NAND (1U << 2) /**< NAND mode enable */
#define MIL_EBC_CONTROL_CPOL (1U << 3) /**< CLOCK polarity */
#define MIL_EBC_CONTROL_BUSY (1U << 7) /**< NAND busy status */

#define MIL_EBC_CONTROL_WAIT_STATE_Pos 12U
#define MIL_EBC_CONTROL_WAIT_STATE_Msk (0xFU << MIL_EBC_CONTROL_WAIT_STATE_Pos)
/** @} */

/** @defgroup MIL_EBC_Window EBC NAND memory window bases
 *  @{
 */
#define MIL_EBC_NAND_CMD_WINDOW_BASE 0x67000000U  /**< SIX - SEVEN! */
#define MIL_EBC_NAND_DATA_WINDOW_BASE 0x67080000U /**< SIX - SEVEN! */
/** @} */

/* Exported types ------------------------------------------------------------*/

/**
 * @brief EBC operating mode
 */
typedef enum {
  MIL_EBC_MODE_NONE = 0,                   /**< All modes disabled */
  MIL_EBC_MODE_ROM  = MIL_EBC_CONTROL_ROM, /**< ROM mode */
  MIL_EBC_MODE_RAM  = MIL_EBC_CONTROL_RAM, /**< RAM mode */
  MIL_EBC_MODE_NAND = MIL_EBC_CONTROL_NAND /**< NAND mode */
} MIL_EBC_Mode;

/**
 * @brief NAND timing parameters (4-bit fields, HCLK cycles)
 */
typedef struct {
  uint8_t t_rr;   /**< Ready to RE# low time (0-15) */
  uint8_t t_alea; /**< ALE to data start time (0-15) */
  uint8_t t_whr;  /**< WE# high to RE# low time (0-15) */
  uint8_t t_wp;   /**< WE# pulse width (0-15) */
  uint8_t t_rea;  /**< RE# pulse width (0-15) */
  uint8_t t_wc;   /**< WE# cycle time (0-15) */
  uint8_t t_rc;   /**< RE# cycle time (0-15) */
} MIL_EBC_NandCycles;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  Enable EBC peripheral clock and reset registers
 * @return None
 */
void MIL_EBC_Init(void);

/**
 * @brief  Disable EBC and clear all control settings
 * @return None
 */
void MIL_EBC_DeInit(void);

/**
 * @brief  Hard reset EBC: save state, disable clock, zero regs, restore state
 * @return None
 */
void MIL_EBC_ReInit(void);

/**
 * @brief  Select EBC operating mode
 * @note   Only one mode can be active at a time. Passing MODE_NONE disables
 *         all.
 * @param  mode  Desired operating mode
 * @return None
 */
void MIL_EBC_EnableMode(MIL_EBC_Mode mode);

/**
 * @brief  Set WAIT_STATE for ROM/RAM modes
 * @param  wait_state  Wait state value (0-15). 0 = 3 HCLK cycles, 15 = 17
 *                     cycles.
 * @return None
 */
void MIL_EBC_SetWaitState(uint8_t wait_state);

/**
 * @brief  Set NAND Flash timing parameters
 * @param  cycles  Pointer to timing structure
 * @return None
 */
void MIL_EBC_SetNandCycles(const MIL_EBC_NandCycles* cycles);

/**
 * @brief  Read NAND busy status from EBC controller
 * @return true if busy, false if ready
 */
bool MIL_EBC_GetBusy(void);

/**
 * @brief  Build encoded NAND command address for EBC address phase
 * @param  addr_cycles      Number of address cycles (0-7)
 * @param  start_cmd        Start command byte (SCMD)
 * @param  end_cmd          End command byte (ECMD)
 * @param  execute_end_cmd  true to execute end command
 * @param  data_phase       true for data phase window
 * @return Encoded address value
 */
uint32_t MIL_EBC_NandBuildCommandAddr(uint8_t addr_cycles, uint8_t start_cmd, uint8_t end_cmd, bool execute_end_cmd, bool data_phase);

/**
 * @brief  Get pointer to NAND data window for byte access
 * @return Volatile pointer to data window base
 */
static inline volatile uint8_t* MIL_EBC_GetNandDataWindow(void) { return (volatile uint8_t*)MIL_EBC_NAND_DATA_WINDOW_BASE; }

#ifdef __cplusplus
}
#endif

#endif /* MIL_EBC_H */
