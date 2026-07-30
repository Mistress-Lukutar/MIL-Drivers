/**
 * @file MIL_ADC.c
 * @brief Implementation of Milandr 1986VE9x ADC peripheral library
 * @author Mistress-Lukutar
 * @date 2025-10-13
 * @version v1.0.0
 */

#include "MIL_ADC.h"

/* ========================================================================== */
/*                         PUBLIC FUNCTION IMPLEMENTATIONS                    */
/* ========================================================================== */

uint8_t MIL_ADC_Init(MIL_ADC_Handle* handle, MIL_ADC_Config* config) {
  if (handle == NULL || config == NULL) {
    return 0;
  }

  /* Copy configuration */
  handle->config = *config;

  /* Set register pointers based on ADC number */
  if (config->adc_number == MIL_ADC1) {
    handle->cfg_reg     = &MDR_ADC->ADC1_CFG;
    handle->h_level_reg = &MDR_ADC->ADC1_H_LEVEL;
    handle->l_level_reg = &MDR_ADC->ADC1_L_LEVEL;
    handle->result_reg  = &MDR_ADC->ADC1_RESULT;
    handle->status_reg  = &MDR_ADC->ADC1_STATUS;
    handle->chsel_reg   = &MDR_ADC->ADC1_CHSEL;
  } else {
    handle->cfg_reg     = &MDR_ADC->ADC2_CFG;
    handle->h_level_reg = &MDR_ADC->ADC2_H_LEVEL;
    handle->l_level_reg = &MDR_ADC->ADC2_L_LEVEL;
    handle->result_reg  = &MDR_ADC->ADC2_RESULT;
    handle->status_reg  = &MDR_ADC->ADC2_STATUS;
    handle->chsel_reg   = &MDR_ADC->ADC2_CHSEL;
  }

  /* Configure ADC */
  uint32_t cfg_value = 0;

  /* Set clock source */
  if (config->clock_source == MIL_ADC_CLOCK_ADC) {
    cfg_value |= ADC1_CFG_REG_CLKS;
  }

  /* Set clock divider */
  if (config->clock_divider > 11) {
    config->clock_divider = 0;
  }
  cfg_value |= (config->clock_divider << ADC1_CFG_REG_DIVCLK_Pos) & ADC1_CFG_REG_DIVCLK_Msk;

  /* Set reference source */
  if (config->ref_source == MIL_ADC_REF_EXTERNAL) {
    cfg_value |= ADC1_CFG_M_REF;
  }

  /* Set conversion mode */
  if (config->conv_mode == MIL_ADC_MODE_CONTINUOUS) {
    cfg_value |= ADC1_CFG_REG_SAMPLE;
  }

  /* Set delay before conversion */
  if (config->delay_go > 7) {
    config->delay_go = 0;
  }
  cfg_value |= (config->delay_go << ADC1_CFG_DELAY_GO_Pos) & ADC1_CFG_DELAY_GO_Msk;

  /* Set channel */
  cfg_value |= (config->channel << ADC1_CFG_REG_CHS_Pos) & ADC1_CFG_REG_CHS_Msk;

  /* Temperature sensor configuration (ADC1 only) */
  if (config->adc_number == MIL_ADC1) {
    if (config->enable_ts) {
      cfg_value |= ADC1_CFG_TS_EN;
    }
    if (config->enable_ts_buffer) {
      cfg_value |= ADC1_CFG_TS_BUF_EN;
    }
    if (config->vref_trim > 15) {
      config->vref_trim = 0;
    }
    cfg_value |= (config->vref_trim << ADC1_CFG_TR_Pos) & ADC1_CFG_TR_Msk;
  }

  /* ADC2 specific: use temperature sensor reference */
  if (config->adc_number == MIL_ADC2 && config->use_ts_vref) {
    cfg_value |= ADC2_CFG_ADC2_OP;
  }

  /* Write configuration */
  *handle->cfg_reg = cfg_value;

  return 1;
}

void MIL_ADC_Enable(MIL_ADC_Handle* handle) {
  if (handle == NULL) {
    return;
  }

  *handle->cfg_reg |= ADC1_CFG_REG_ADON;
}

void MIL_ADC_Disable(MIL_ADC_Handle* handle) {
  if (handle == NULL) {
    return;
  }

  *handle->cfg_reg &= ~ADC1_CFG_REG_ADON;
}

void MIL_ADC_StartConversion(MIL_ADC_Handle* handle) {
  if (handle == NULL) {
    return;
  }

  *handle->cfg_reg |= ADC1_CFG_REG_GO;
}

uint8_t MIL_ADC_IsConversionComplete(MIL_ADC_Handle* handle) {
  if (handle == NULL) {
    return 0;
  }

  return (*handle->status_reg & ADC_STATUS_FLG_REG_EOCIF) ? 1 : 0;
}

uint8_t MIL_ADC_GetResult(MIL_ADC_Handle* handle, MIL_ADC_Result* result) {
  if (handle == NULL || result == NULL) {
    return 0;
  }

  /* Check if conversion is complete */
  if (!MIL_ADC_IsConversionComplete(handle)) {
    return 0;
  }

  /* Read result register */
  uint32_t reg_value = *handle->result_reg;

  /* Extract conversion value */
  result->value = (uint16_t)(reg_value & ADC_RESULT_Msk);

  /* Extract channel number */
  result->channel = (uint8_t)((reg_value & ADC_RESULT_CHANNEL_Msk) >> ADC_RESULT_CHANNEL_Pos);

  /* Reading result automatically clears EOCIF flag */

  return 1;
}

void MIL_ADC_SetChannel(MIL_ADC_Handle* handle, uint8_t channel) {
  if (handle == NULL || channel > 31) {
    return;
  }

  /* Clear channel bits and set new channel */
  uint32_t cfg = *handle->cfg_reg;
  cfg &= ~ADC1_CFG_REG_CHS_Msk;
  cfg |= (channel << ADC1_CFG_REG_CHS_Pos) & ADC1_CFG_REG_CHS_Msk;
  *handle->cfg_reg = cfg;

  /* Update configuration */
  handle->config.channel = channel;
}

void MIL_ADC_ConfigureChannelScan(MIL_ADC_Handle* handle, uint32_t channel_mask, uint8_t enable) {
  if (handle == NULL) {
    return;
  }

  /* Set channel selection mask */
  *handle->chsel_reg = channel_mask;

  /* Enable or disable channel switching */
  if (enable) {
    *handle->cfg_reg |= ADC1_CFG_REG_CHCH;
  } else {
    *handle->cfg_reg &= ~ADC1_CFG_REG_CHCH;
  }
}

void MIL_ADC_SetBoundaryLevels(MIL_ADC_Handle* handle, uint16_t lower_level, uint16_t upper_level, uint8_t enable) {
  if (handle == NULL) {
    return;
  }

  /* Set lower and upper levels (12-bit values) */
  *handle->l_level_reg = lower_level & 0xFFF;
  *handle->h_level_reg = upper_level & 0xFFF;

  /* Enable or disable range checking */
  if (enable) {
    *handle->cfg_reg |= ADC1_CFG_REG_RNGC;
  } else {
    *handle->cfg_reg &= ~ADC1_CFG_REG_RNGC;
  }
}

uint8_t MIL_ADC_IsOutOfRange(MIL_ADC_Handle* handle) {
  if (handle == NULL) {
    return 0;
  }

  return (*handle->status_reg & ADC_STATUS_FLG_REG_AWOIFEN) ? 1 : 0;
}

uint8_t MIL_ADC_IsOverwritten(MIL_ADC_Handle* handle) {
  if (handle == NULL) {
    return 0;
  }

  return (*handle->status_reg & ADC_STATUS_FLG_REG_OVERWRITE) ? 1 : 0;
}

void MIL_ADC_ClearFlags(MIL_ADC_Handle* handle, uint32_t flags) {
  if (handle == NULL) {
    return;
  }

  /* Write zeros to clear flags (OVERWRITE and AWOIFEN) */
  uint32_t status = *handle->status_reg;
  status &= ~(flags & (ADC_STATUS_FLG_REG_OVERWRITE | ADC_STATUS_FLG_REG_AWOIFEN));
  *handle->status_reg = status;
}

void MIL_ADC_EnableInterrupts(MIL_ADC_Handle* handle, uint8_t eoc_int, uint8_t awoi_int) {
  if (handle == NULL) {
    return;
  }

  uint32_t status = *handle->status_reg;

  if (eoc_int) {
    status |= ADC_STATUS_ECOIF_IE;
  }

  if (awoi_int) {
    status |= ADC_STATUS_AWOIF_IE;
  }

  *handle->status_reg = status;
}

void MIL_ADC_DisableInterrupts(MIL_ADC_Handle* handle, uint8_t eoc_int, uint8_t awoi_int) {
  if (handle == NULL) {
    return;
  }

  uint32_t status = *handle->status_reg;

  if (eoc_int) {
    status &= ~ADC_STATUS_ECOIF_IE;
  }

  if (awoi_int) {
    status &= ~ADC_STATUS_AWOIF_IE;
  }

  *handle->status_reg = status;
}

void MIL_ADC_ConfigureSyncMode(MIL_ADC_Handle* handle1, MIL_ADC_Handle* handle2, uint8_t delay_adc, uint8_t enable) {
  if (handle1 == NULL || handle2 == NULL) {
    return;
  }

  /* Only ADC1 can initiate synchronous mode */
  if (handle1->config.adc_number != MIL_ADC1) {
    return;
  }

  /* Set delay between ADC1 and ADC2 */
  if (delay_adc > 15) {
    delay_adc = 0;
  }

  uint32_t cfg1 = *handle1->cfg_reg;
  cfg1 &= ~ADC1_CFG_DELAY_ADC_Msk;
  cfg1 |= (delay_adc << ADC1_CFG_DELAY_ADC_Pos) & ADC1_CFG_DELAY_ADC_Msk;

  /* Enable or disable synchronous mode */
  if (enable) {
    cfg1 |= ADC1_CFG_SYNC_CONVER;
  } else {
    cfg1 &= ~ADC1_CFG_SYNC_CONVER;
  }

  *handle1->cfg_reg = cfg1;
}

uint8_t MIL_ADC_ReadTemperature(MIL_ADC_Handle* handle, MIL_ADC_Result* result) {
  if (handle == NULL || result == NULL) {
    return 0;
  }

  /* Temperature sensor only available on ADC1 */
  if (handle->config.adc_number != MIL_ADC1) {
    return 0;
  }

  /* Configure for temperature sensor reading */
  uint32_t cfg = *handle->cfg_reg;

  /* Set channel 31 for temperature sensor */
  cfg &= ~ADC1_CFG_REG_CHS_Msk;
  cfg |= (MIL_ADC_CHANNEL_TEMP << ADC1_CFG_REG_CHS_Pos) & ADC1_CFG_REG_CHS_Msk;

  /* Enable temperature sensor and buffer */
  cfg |= ADC1_CFG_TS_EN | ADC1_CFG_TS_BUF_EN | ADC1_CFG_SEL_TS;

  /* Disable channel switching for single channel conversion */
  cfg &= ~ADC1_CFG_REG_CHCH;

  *handle->cfg_reg = cfg;

  /* Start conversion */
  MIL_ADC_StartConversion(handle);

  /* Wait for conversion to complete */
  uint32_t timeout = 100000;
  while (!MIL_ADC_IsConversionComplete(handle) && timeout > 0) {
    timeout--;
  }

  if (timeout == 0) {
    return 0;
  }

  /* Get result */
  return MIL_ADC_GetResult(handle, result);
}

uint8_t MIL_ADC_ReadVoltageRef(MIL_ADC_Handle* handle, MIL_ADC_Result* result) {
  if (handle == NULL || result == NULL) {
    return 0;
  }

  /* Voltage reference only available on ADC1 */
  if (handle->config.adc_number != MIL_ADC1) {
    return 0;
  }

  /* Configure for voltage reference reading */
  uint32_t cfg = *handle->cfg_reg;

  /* Set channel 30 for voltage reference */
  cfg &= ~ADC1_CFG_REG_CHS_Msk;
  cfg |= (MIL_ADC_CHANNEL_VREF << ADC1_CFG_REG_CHS_Pos) & ADC1_CFG_REG_CHS_Msk;

  /* Enable temperature sensor (provides voltage reference) and buffer */
  cfg |= ADC1_CFG_TS_EN | ADC1_CFG_TS_BUF_EN | ADC1_CFG_SEL_VREF;

  /* Disable channel switching for single channel conversion */
  cfg &= ~ADC1_CFG_REG_CHCH;

  *handle->cfg_reg = cfg;

  /* Start conversion */
  MIL_ADC_StartConversion(handle);

  /* Wait for conversion to complete */
  uint32_t timeout = 100000;
  while (!MIL_ADC_IsConversionComplete(handle) && timeout > 0) {
    timeout--;
  }

  if (timeout == 0) {
    return 0;
  }

  /* Get result */
  return MIL_ADC_GetResult(handle, result);
}

uint8_t MIL_ADC_ConvertChannel(MIL_ADC_Handle* handle, uint8_t channel, MIL_ADC_Result* result, uint32_t timeout) {
  if (handle == NULL || result == NULL || channel > 31) {
    return 0;
  }

  /* Set channel */
  MIL_ADC_SetChannel(handle, channel);

  /* Disable channel switching for single conversion */
  *handle->cfg_reg &= ~ADC1_CFG_REG_CHCH;

  /* Start conversion */
  MIL_ADC_StartConversion(handle);

  /* Wait for conversion to complete */
  while (!MIL_ADC_IsConversionComplete(handle) && timeout > 0) {
    timeout--;
  }

  if (timeout == 0) {
    return 0;
  }

  /* Get result */
  return MIL_ADC_GetResult(handle, result);
}

void MIL_ADC_SetScanDelay(MIL_ADC_Handle* handle, uint8_t delay) {
  if (handle == NULL) {
    return;
  }

  /* Scan delay only available on ADC2 */
  if (handle->config.adc_number != MIL_ADC2) {
    return;
  }

  /* Limit delay value */
  if (delay > 7) {
    delay = 0;
  }

  /* Set delay between conversions in scan mode */
  uint32_t cfg = *handle->cfg_reg;
  cfg &= ~ADC2_CFG_DELAY_GO_Msk;
  cfg |= (delay << ADC2_CFG_DELAY_GO_Pos) & ADC2_CFG_DELAY_GO_Msk;
  *handle->cfg_reg = cfg;
}

void MIL_ADC_EnableNVIC(void) { NVIC_EnableIRQ(ADC_IRQn); }

void MIL_ADC_DisableNVIC(void) { NVIC_DisableIRQ(ADC_IRQn); }
