/**
 * @file MIL_TIMx.c
 * @brief Timer and PWM control library implementation for MDR32F9Qx
 * @author Mistress-Lukutar
 * @date 2025-10-14
 * @version v1.0.0
 */

#include "MIL_TIMx.h"

/* ==================== Private Helper Functions ==================== */

/**
 * @brief Get timer number from pointer
 * @param TIMERx Pointer to timer peripheral
 * @return Timer number (1, 2, or 3)
 */
static uint8_t MIL_TIMx_GetNumber(MDR_TIMER_TypeDef* TIMERx) {
  if (TIMERx == MDR_TIMER1)
    return 1;
  if (TIMERx == MDR_TIMER2)
    return 2;
  if (TIMERx == MDR_TIMER3)
    return 3;
  return 0;
}

/* ==================== Clock Control Functions ==================== */

/**
 * @brief Enable timer clock with specified divider
 * @param TIMERx Pointer to timer peripheral
 * @param clock_div Clock divider value
 */
void MIL_TIMx_EnableClock(MDR_TIMER_TypeDef* TIMERx, uint8_t clock_div) {
  uint8_t timer_num = MIL_TIMx_GetNumber(TIMERx);
  uint32_t temp;

  /* Enable peripheral clock */
  if (timer_num == 1) {
    MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_TIMER1;
    MDR_RST_CLK->TIM_CLOCK |= RST_CLK_TIM_CLOCK_TIM1_CLK_EN;
  } else if (timer_num == 2) {
    MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_TIMER2;
    MDR_RST_CLK->TIM_CLOCK |= RST_CLK_TIM_CLOCK_TIM2_CLK_EN;

  } else if (timer_num == 3) {
    MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_TIMER3;
    MDR_RST_CLK->TIM_CLOCK |= RST_CLK_TIM_CLOCK_TIM3_CLK_EN;
  }

  /* Configure timer clock */
  temp = MDR_RST_CLK->TIM_CLOCK;

  if (timer_num == 1) {
    temp &= ~RST_CLK_TIM_CLOCK_TIM1_BRG_Msk;
    temp |= (clock_div << RST_CLK_TIM_CLOCK_TIM1_BRG_Pos);
    temp |= RST_CLK_TIM_CLOCK_TIM1_CLK_EN;
  } else if (timer_num == 2) {
    temp &= ~RST_CLK_TIM_CLOCK_TIM2_BRG_Msk;
    temp |= (clock_div << RST_CLK_TIM_CLOCK_TIM2_BRG_Pos);
    temp |= RST_CLK_TIM_CLOCK_TIM2_CLK_EN;
  } else if (timer_num == 3) {
    temp &= ~RST_CLK_TIM_CLOCK_TIM3_BRG_Msk;
    temp |= (clock_div << RST_CLK_TIM_CLOCK_TIM3_BRG_Pos);
    temp |= RST_CLK_TIM_CLOCK_TIM3_CLK_EN;
  }

  MDR_RST_CLK->TIM_CLOCK = temp;
}

/**
 * @brief Disable timer clock
 * @param TIMERx Pointer to timer peripheral
 */
void MIL_TIMx_DisableClock(MDR_TIMER_TypeDef* TIMERx) {
  uint8_t timer_num = MIL_TIMx_GetNumber(TIMERx);

  if (timer_num == 1) {
    MDR_RST_CLK->TIM_CLOCK &= ~RST_CLK_TIM_CLOCK_TIM1_CLK_EN;
  } else if (timer_num == 2) {
    MDR_RST_CLK->TIM_CLOCK &= ~RST_CLK_TIM_CLOCK_TIM2_CLK_EN;
  } else if (timer_num == 3) {
    MDR_RST_CLK->TIM_CLOCK &= ~RST_CLK_TIM_CLOCK_TIM3_CLK_EN;
  }
}

/* ==================== Initialization Functions ==================== */

/**
 * @brief Initialize timer with base configuration
 * @param TIMERx Pointer to timer peripheral
 * @param init Pointer to initialization structure
 */
void MIL_TIMx_Init(MDR_TIMER_TypeDef* TIMERx, MIL_TIMx_InitTypeDef* init) {
  uint32_t temp;

  /* Disable timer during configuration */
  TIMERx->CNTRL = 0;

  /* Set prescaler */
  TIMERx->PSG = init->prescaler;

  /* Set period (auto-reload register) */
  TIMERx->ARR = init->period;

  /* Set initial counter value */
  TIMERx->CNT = 0;

  /* Configure control register */
  temp = 0;
  temp |= (init->counter_mode << TIMER_CNTRL_CNT_MODE_Pos) & TIMER_CNTRL_CNT_MODE_Msk;
  temp |= (init->event_source << TIMER_CNTRL_EVENT_SEL_Pos) & TIMER_CNTRL_EVENT_SEL_Msk;

  TIMERx->CNTRL = temp;
}

/**
 * @brief Reset timer to default state
 * @param TIMERx Pointer to timer peripheral
 */
void MIL_TIMx_DeInit(MDR_TIMER_TypeDef* TIMERx) {
  TIMERx->CNTRL  = 0;
  TIMERx->CNT    = 0;
  TIMERx->PSG    = 0;
  TIMERx->ARR    = 0;
  TIMERx->IE     = 0;
  TIMERx->STATUS = 0xFFFFFFFF; /* Clear all flags */

  /* Reset all channels */
  for (int i = 0; i < 4; i++) {
    TIMERx->CH1_CNTRL  = 0;
    TIMERx->CH1_CNTRL1 = 0;
    TIMERx->CH1_CNTRL2 = 0;
    TIMERx->CH1_DTG    = 0;
    TIMERx->CCR1       = 0;
    TIMERx->CCR11      = 0;
  }

  TIMERx->BRKETR_CNTRL = 0;
}

/* ==================== Counter Control Functions ==================== */

/**
 * @brief Start timer counter
 * @param TIMERx Pointer to timer peripheral
 */
void MIL_TIMx_Start(MDR_TIMER_TypeDef* TIMERx) { TIMERx->CNTRL |= TIMER_CNTRL_CNT_EN; }

/**
 * @brief Stop timer counter
 * @param TIMERx Pointer to timer peripheral
 */
void MIL_TIMx_Stop(MDR_TIMER_TypeDef* TIMERx) { TIMERx->CNTRL &= ~TIMER_CNTRL_CNT_EN; }

/**
 * @brief Set counter value
 * @param TIMERx Pointer to timer peripheral
 * @param value Counter value to set
 */
void MIL_TIMx_SetCounter(MDR_TIMER_TypeDef* TIMERx, uint16_t value) { TIMERx->CNT = value; }

/**
 * @brief Get current counter value
 * @param TIMERx Pointer to timer peripheral
 * @return Current counter value
 */
uint16_t MIL_TIMx_GetCounter(MDR_TIMER_TypeDef* TIMERx) { return (uint16_t)(TIMERx->CNT & 0xFFFF); }

/**
 * @brief Set timer period (ARR value)
 * @param TIMERx Pointer to timer peripheral
 * @param period Period value
 */
void MIL_TIMx_SetPeriod(MDR_TIMER_TypeDef* TIMERx, uint16_t period) { TIMERx->ARR = period; }

/**
 * @brief Get timer period (ARR value)
 * @param TIMERx Pointer to timer peripheral
 * @return Period value
 */
uint16_t MIL_TIMx_GetPeriod(MDR_TIMER_TypeDef* TIMERx) { return (uint16_t)(TIMERx->ARR & 0xFFFF); }

/**
 * @brief Set timer prescaler
 * @param TIMERx Pointer to timer peripheral
 * @param prescaler Prescaler value
 */
void MIL_TIMx_SetPrescaler(MDR_TIMER_TypeDef* TIMERx, uint16_t prescaler) { TIMERx->PSG = prescaler; }

/* ==================== PWM Functions ==================== */

/**
 * @brief Initialize PWM channel
 * @param TIMERx Pointer to timer peripheral
 * @param pwm_config Pointer to PWM configuration structure
 */
void MIL_TIMx_PWM_Init(MDR_TIMER_TypeDef* TIMERx, PWM_ChannelTypeDef* pwm_config) {
  volatile uint32_t* ch_cntrl  = &TIMERx->CH1_CNTRL + (pwm_config->channel * 4);
  volatile uint32_t* ch_cntrl1 = &TIMERx->CH1_CNTRL1 + (pwm_config->channel * 4);
  volatile uint32_t* ccr       = &TIMERx->CCR1 + (pwm_config->channel);

  uint32_t temp;

  /* Configure channel control register for PWM mode */
  temp = 0;
  temp &= ~TIMER_CH_CNTRL_CAP_NPWM; /* PWM mode */
  temp |= (pwm_config->mode << TIMER_CH_CNTRL_OCCM_Pos) & TIMER_CH_CNTRL_OCCM_Msk;
  *ch_cntrl = temp;

  /* Configure output control */
  temp = 0;
  temp |= (pwm_config->output_enable << TIMER_CH_CNTRL1_SELOE_Pos) & TIMER_CH_CNTRL1_SELOE_Msk;
  temp |= (TIMER_CH_CNTRL1_SELO_OUT_REF << TIMER_CH_CNTRL1_SELO_Pos) & TIMER_CH_CNTRL1_SELO_Msk;

  if (pwm_config->polarity) {
    temp |= TIMER_CH_CNTRL1_INV;
  }

  /* Configure complementary output if enabled */
  if (pwm_config->complementary_enable) {
    temp |= (pwm_config->output_enable << TIMER_CH_CNTRL1_NSELOE_Pos) & TIMER_CH_CNTRL1_NSELOE_Msk;
    temp |= (TIMER_CH_CNTRL1_SELO_OUT_REF << TIMER_CH_CNTRL1_NSELO_Pos) & TIMER_CH_CNTRL1_NSELO_Msk;
  }

  *ch_cntrl1 = temp;

  /* Set pulse width */
  *ccr = pwm_config->pulse;
}

/**
 * @brief Set PWM duty cycle (absolute value)
 * @param TIMERx Pointer to timer peripheral
 * @param channel Channel number (0-3)
 * @param pulse Pulse width value
 */
void MIL_TIMx_PWM_SetDutyCycle(MDR_TIMER_TypeDef* TIMERx, uint8_t channel, uint16_t pulse) {
  volatile uint32_t* ccr = &TIMERx->CCR1 + channel;
  *ccr                   = pulse;
}

/**
 * @brief Set PWM duty cycle (percentage)
 * @param TIMERx Pointer to timer peripheral
 * @param channel Channel number (0-3)
 * @param percent Duty cycle percentage (0-100)
 */
void MIL_TIMx_PWM_SetDutyCyclePercent(MDR_TIMER_TypeDef* TIMERx, uint8_t channel, uint8_t percent) {
  uint16_t period = MIL_TIMx_GetPeriod(TIMERx);
  uint16_t pulse  = (uint16_t)((uint32_t)period * percent / 100);
  MIL_TIMx_PWM_SetDutyCycle(TIMERx, channel, pulse);
}

/**
 * @brief Start PWM output on channel
 * @param TIMERx Pointer to timer peripheral
 * @param channel Channel number (0-3)
 */
void MIL_TIMx_PWM_Start(MDR_TIMER_TypeDef* TIMERx, uint8_t channel) {
  volatile uint32_t* ch_cntrl1 = &TIMERx->CH1_CNTRL1 + (channel * 4);
  *ch_cntrl1 |= (OUTPUT_ENABLED << TIMER_CH_CNTRL1_SELOE_Pos);
}

/**
 * @brief Stop PWM output on channel
 * @param TIMERx Pointer to timer peripheral
 * @param channel Channel number (0-3)
 */
void MIL_TIMx_PWM_Stop(MDR_TIMER_TypeDef* TIMERx, uint8_t channel) {
  volatile uint32_t* ch_cntrl1 = &TIMERx->CH1_CNTRL1 + (channel * 4);
  *ch_cntrl1 &= ~TIMER_CH_CNTRL1_SELOE_Msk;
}

/* ==================== Dead-Time Functions ==================== */

/**
 * @brief Configure dead-time generator
 * @param TIMERx Pointer to timer peripheral
 * @param dtg_config Pointer to dead-time configuration
 */
void MIL_TIMx_DeadTime_Config(MDR_TIMER_TypeDef* TIMERx, DeadTime_ConfigTypeDef* dtg_config) {
  volatile uint32_t* ch_dtg    = &TIMERx->CH1_DTG + (dtg_config->channel * 4);
  volatile uint32_t* ch_cntrl1 = &TIMERx->CH1_CNTRL1 + (dtg_config->channel * 4);

  uint32_t temp;

  /* Configure DTG register */
  temp = 0;
  temp |= (dtg_config->dtg_prescaler << TIMER_CH_DTGX_Pos) & TIMER_CH_DTGX_Msk;
  temp |= (dtg_config->dtg_value << TIMER_CH_DTG_Pos) & TIMER_CH_DTG_Msk;

  if (dtg_config->use_fdts) {
    temp |= TIMER_CH_DTG_EDTS;
  }

  *ch_dtg = temp;

  /* Enable DTG output in control register */
  temp = *ch_cntrl1;
  temp &= ~TIMER_CH_CNTRL1_SELO_Msk;
  temp |= (TIMER_CH_CNTRL1_SELO_OUT_DTG << TIMER_CH_CNTRL1_SELO_Pos);
  temp &= ~TIMER_CH_CNTRL1_NSELO_Msk;
  temp |= (TIMER_CH_CNTRL1_SELO_OUT_DTG << TIMER_CH_CNTRL1_NSELO_Pos);
  *ch_cntrl1 = temp;
}

/**
 * @brief Set dead-time value
 * @param TIMERx Pointer to timer peripheral
 * @param channel Channel number (0-3)
 * @param dtg_value Dead-time value (0-255)
 */
void MIL_TIMx_DeadTime_SetValue(MDR_TIMER_TypeDef* TIMERx, uint8_t channel, uint8_t dtg_value) {
  volatile uint32_t* ch_dtg = &TIMERx->CH1_DTG + (channel * 4);
  uint32_t temp             = *ch_dtg;

  temp &= ~TIMER_CH_DTG_Msk;
  temp |= (dtg_value << TIMER_CH_DTG_Pos) & TIMER_CH_DTG_Msk;

  *ch_dtg = temp;
}

/**
 * @brief Get dead-time value
 * @param TIMERx Pointer to timer peripheral
 * @param channel Channel number (0-3)
 * @return Dead-time value (0-255)
 */
uint8_t MIL_TIMx_DeadTime_GetValue(MDR_TIMER_TypeDef* TIMERx, uint8_t channel) {
  volatile uint32_t* ch_dtg = &TIMERx->CH1_DTG + (channel * 4);
  return (*ch_dtg & TIMER_CH_DTG_Msk) >> TIMER_CH_DTG_Pos;
}

/* ==================== Capture Functions ==================== */

/**
 * @brief Initialize capture mode on channel
 * @param TIMERx Pointer to timer peripheral
 * @param cap_config Pointer to capture configuration
 */
void MIL_TIMx_Capture_Init(MDR_TIMER_TypeDef* TIMERx, Capture_ConfigTypeDef* cap_config) {
  volatile uint32_t* ch_cntrl  = &TIMERx->CH1_CNTRL + (cap_config->channel * 4);
  volatile uint32_t* ch_cntrl1 = &TIMERx->CH1_CNTRL1 + (cap_config->channel * 4);

  uint32_t temp;

  /* Configure channel for capture mode */
  temp = 0;
  temp |= TIMER_CH_CNTRL_CAP_NPWM; /* Capture mode */
  temp |= (cap_config->filter << TIMER_CH_CNTRL_CHFLTR_Pos) & TIMER_CH_CNTRL_CHFLTR_Msk;
  temp |= (cap_config->edge_select << TIMER_CH_CNTRL_CHSEL_Pos) & TIMER_CH_CNTRL_CHSEL_Msk;
  temp |= (cap_config->prescaler << TIMER_CH_CNTRL_CHPSC_Pos) & TIMER_CH_CNTRL_CHPSC_Msk;

  *ch_cntrl = temp;

  /* Disable output for capture mode */
  *ch_cntrl1 = 0;
}

/**
 * @brief Get captured value from CCR register
 * @param TIMERx Pointer to timer peripheral
 * @param channel Channel number (0-3)
 * @return Captured counter value
 */
uint16_t MIL_TIMx_Capture_GetValue(MDR_TIMER_TypeDef* TIMERx, uint8_t channel) {
  volatile uint32_t* ccr = &TIMERx->CCR1 + channel;
  return (uint16_t)(*ccr & 0xFFFF);
}

/**
 * @brief Get captured value from CCR1 register
 * @param TIMERx Pointer to timer peripheral
 * @param channel Channel number (0-3)
 * @return Captured counter value from CCR1
 */
uint16_t MIL_TIMx_Capture_GetValue1(MDR_TIMER_TypeDef* TIMERx, uint8_t channel) {
  volatile uint32_t* ccr1 = &TIMERx->CCR11 + channel;
  return (uint16_t)(*ccr1 & 0xFFFF);
}

/* ==================== Interrupt Functions ==================== */

/**
 * @brief Configure timer interrupts
 * @param TIMERx Pointer to timer peripheral
 * @param interrupt_mask Interrupt sources mask
 * @param enable Enable (1) or disable (0) interrupts
 */
void MIL_TIMx_ITConfig(MDR_TIMER_TypeDef* TIMERx, uint32_t interrupt_mask, uint8_t enable) {
  if (enable) {
    TIMERx->IE |= interrupt_mask;
  } else {
    TIMERx->IE &= ~interrupt_mask;
  }
}

/**
 * @brief Get interrupt status
 * @param TIMERx Pointer to timer peripheral
 * @param interrupt_mask Interrupt sources mask to check
 * @return Non-zero if any specified interrupt is pending
 */
uint32_t MIL_TIMx_GetITStatus(MDR_TIMER_TypeDef* TIMERx, uint32_t interrupt_mask) { return (TIMERx->STATUS & interrupt_mask); }

/**
 * @brief Clear interrupt pending bits
 * @param TIMERx Pointer to timer peripheral
 * @param interrupt_mask Interrupt sources mask to clear
 */
void MIL_TIMx_ClearITPendingBit(MDR_TIMER_TypeDef* TIMERx, uint32_t interrupt_mask) {
  /* Write 0 to clear flags (according to datasheet) */
  TIMERx->STATUS &= ~interrupt_mask;
}

/* ==================== Break and ETR Functions ==================== */

/**
 * @brief Configure break input
 * @param TIMERx Pointer to timer peripheral
 * @param polarity Break input polarity (0=normal, 1=inverted)
 */
void MIL_TIMx_BRK_Config(MDR_TIMER_TypeDef* TIMERx, uint8_t polarity) {
  if (polarity) {
    TIMERx->BRKETR_CNTRL |= TIMER_BRKETR_CNTRL_BRK_INV;
  } else {
    TIMERx->BRKETR_CNTRL &= ~TIMER_BRKETR_CNTRL_BRK_INV;
  }
}

/**
 * @brief Configure external trigger input (ETR)
 * @param TIMERx Pointer to timer peripheral
 * @param polarity ETR polarity (0=normal, 1=inverted)
 * @param prescaler ETR prescaler (0-3)
 * @param filter ETR filter value (0-15)
 */
void MIL_TIMx_ETR_Config(MDR_TIMER_TypeDef* TIMERx, uint8_t polarity, uint8_t prescaler, uint8_t filter) {
  uint32_t temp = 0;

  if (polarity) {
    temp |= TIMER_BRKETR_CNTRL_ETR_INV;
  }

  temp |= (prescaler << TIMER_BRKETR_CNTRL_ETR_PSC_Pos) & TIMER_BRKETR_CNTRL_ETR_PSC_Msk;
  temp |= (filter << TIMER_BRKETR_CNTRL_ETR_FILTER_Pos) & TIMER_BRKETR_CNTRL_ETR_FILTER_Msk;

  TIMERx->BRKETR_CNTRL = temp;
}

/**
 * @brief Configure ETR input filter only
 * @param TIMERx Pointer to timer peripheral
 * @param filter ETR filter value (0-15)
 */
void MIL_TIMx_ETR_FilterConfig(MDR_TIMER_TypeDef* TIMERx, uint8_t filter) {
  uint32_t temp = TIMERx->BRKETR_CNTRL;

  temp &= ~TIMER_BRKETR_CNTRL_ETR_FILTER_Msk;
  temp |= (filter << TIMER_BRKETR_CNTRL_ETR_FILTER_Pos) & TIMER_BRKETR_CNTRL_ETR_FILTER_Msk;

  TIMERx->BRKETR_CNTRL = temp;
}
