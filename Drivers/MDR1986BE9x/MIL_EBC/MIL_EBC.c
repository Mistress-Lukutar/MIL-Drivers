/**
 * @file   MIL_EBC.c
 * @brief  External Bus Controller (EBC) driver implementation
 * @author Mistress-Lukutar
 * @date   2026-05-19
 * @version v1.0.0
 */

/* Includes ------------------------------------------------------------------*/
#include "MIL_EBC.h"

/* Private constants ---------------------------------------------------------*/

#define _NAND_ADDR_ADR_CYCLES_SHIFT 21U
#define _NAND_ADDR_END_CMD_SHIFT 20U
#define _NAND_ADDR_DATA_PHASE_SHIFT 19U
#define _NAND_ADDR_ECMD_SHIFT 11U
#define _NAND_ADDR_SCMD_SHIFT 3U

/* Public functions ----------------------------------------------------------*/

void MIL_EBC_Init(void) {
  MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_EXT_BUS_CNTRL;
  MDR_EBC->CONTROL     = 0;
  MDR_EBC->NAND_CYCLES = 0;
}

void MIL_EBC_DeInit(void) {
  MDR_EBC->CONTROL     = 0;
  MDR_EBC->NAND_CYCLES = 0;
}

void MIL_EBC_ReInit(void) {
  uint32_t ctrl   = MDR_EBC->CONTROL;
  uint32_t cycles = MDR_EBC->NAND_CYCLES;

  MDR_RST_CLK->PER_CLOCK &= ~RST_CLK_PER_CLOCK_PCLK_EN_EXT_BUS_CNTRL;
  MDR_EBC->CONTROL     = 0;
  MDR_EBC->NAND_CYCLES = 0;

  MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_EXT_BUS_CNTRL;
  MDR_EBC->CONTROL     = ctrl;
  MDR_EBC->NAND_CYCLES = cycles;
}

void MIL_EBC_EnableMode(MIL_EBC_Mode mode) {
  uint32_t ctrl = MDR_EBC->CONTROL;

  ctrl &= ~(MIL_EBC_CONTROL_ROM | MIL_EBC_CONTROL_RAM | MIL_EBC_CONTROL_NAND);
  ctrl |= (uint32_t)mode;

  MDR_EBC->CONTROL = ctrl;
}

void MIL_EBC_SetWaitState(uint8_t wait_state) {
  uint32_t ctrl = MDR_EBC->CONTROL;

  ctrl &= ~MIL_EBC_CONTROL_WAIT_STATE_Msk;
  ctrl |= ((uint32_t)(wait_state & 0xFU) << MIL_EBC_CONTROL_WAIT_STATE_Pos);

  MDR_EBC->CONTROL = ctrl;
}

void MIL_EBC_SetNandCycles(const MIL_EBC_NandCycles* cycles) {
  uint32_t reg = 0;

  reg |= ((uint32_t)(cycles->t_rr & 0xFU) << 24);
  reg |= ((uint32_t)(cycles->t_alea & 0xFU) << 20);
  reg |= ((uint32_t)(cycles->t_whr & 0xFU) << 16);
  reg |= ((uint32_t)(cycles->t_wp & 0xFU) << 12);
  reg |= ((uint32_t)(cycles->t_rea & 0xFU) << 8);
  reg |= ((uint32_t)(cycles->t_wc & 0xFU) << 4);
  reg |= ((uint32_t)(cycles->t_rc & 0xFU) << 0);

  MDR_EBC->NAND_CYCLES = reg;
}

bool MIL_EBC_GetBusy(void) { return !((MDR_EBC->CONTROL & MIL_EBC_CONTROL_BUSY) != 0); }

uint32_t MIL_EBC_NandBuildCommandAddr(uint8_t addr_cycles, uint8_t start_cmd, uint8_t end_cmd, bool execute_end_cmd, bool data_phase) {
  uint32_t addr = MIL_EBC_NAND_CMD_WINDOW_BASE;

  addr |= ((uint32_t)(addr_cycles & 0x7U) << _NAND_ADDR_ADR_CYCLES_SHIFT);
  addr |= ((uint32_t)start_cmd << _NAND_ADDR_SCMD_SHIFT);
  addr |= ((uint32_t)end_cmd << _NAND_ADDR_ECMD_SHIFT);

  if (execute_end_cmd) {
    addr |= (1U << _NAND_ADDR_END_CMD_SHIFT);
  }

  if (data_phase) {
    addr |= (1U << _NAND_ADDR_DATA_PHASE_SHIFT);
  }

  return addr;
}
