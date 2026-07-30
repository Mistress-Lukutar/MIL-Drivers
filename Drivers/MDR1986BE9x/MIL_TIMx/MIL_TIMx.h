/**
 * @file MIL_TIMx.h
 * @brief Timer and PWM control library for MDR32F9Qx microcontrollers
 * @author Mistress-Lukutar
 * @date 2025-10-14
 * @version v1.0.0
 */

#ifndef MIL_TIMX_H
#define MIL_TIMX_H

#include "MDR32Fx.h"
#include <stdint.h>

/* ==================== Timer Selection Macros ==================== */
/**
 * @defgroup MIL_TIMx_Instance Timer Instance Selection
 * @{
 */
#define TIMx_1 MDR_TIMER1
#define TIMx_2 MDR_TIMER2
#define TIMx_3 MDR_TIMER3
/** @} */

/* ==================== Clock Divider Macros ==================== */
/**
 * @defgroup MIL_TIMx_Clock Timer Clock Configuration
 * @{
 */
#define TIMx_CLK_DIV_1 RST_CLK_TIM_CLOCK_TIM_BRG_HCLK
#define TIMx_CLK_DIV_2 RST_CLK_TIM_CLOCK_TIM_BRG_HCLK_DIV_2
#define TIMx_CLK_DIV_4 RST_CLK_TIM_CLOCK_TIM_BRG_HCLK_DIV_4
#define TIMx_CLK_DIV_8 RST_CLK_TIM_CLOCK_TIM_BRG_HCLK_DIV_8
#define TIMx_CLK_DIV_16 RST_CLK_TIM_CLOCK_TIM_BRG_HCLK_DIV_16
#define TIMx_CLK_DIV_32 RST_CLK_TIM_CLOCK_TIM_BRG_HCLK_DIV_32
#define TIMx_CLK_DIV_64 RST_CLK_TIM_CLOCK_TIM_BRG_HCLK_DIV_64
#define TIMx_CLK_DIV_128 RST_CLK_TIM_CLOCK_TIM_BRG_HCLK_DIV_128
/** @} */

/* ==================== Counter Mode Macros ==================== */
/**
 * @defgroup MIL_TIMx_CountMode Timer Counter Mode
 * @{
 */
#define TIMx_MODE_UP TIMER_CNTRL_CNT_MODE_UP_DIR_0_PSG_0
#define TIMx_MODE_DOWN TIMER_CNTRL_CNT_MODE_DOWN_DIR_1_PSG_0
#define TIMx_MODE_UP_DOWN TIMER_CNTRL_CNT_MODE_UP_DOWN_DIR_AUTO_PSG_0
/** @} */

/* ==================== Event Source Macros ==================== */
/**
 * @defgroup MIL_TIMx_EventSource Timer Event Source
 * @{
 */
#define TIMx_EVENT_TIM_CLK TIMER_CNTRL_EVENT_SEL_TIM_CLK
#define TIMx_EVENT_TIM1 TIMER_CNTRL_EVENT_SEL_CNT_ARR_TIM1
#define TIMx_EVENT_TIM2 TIMER_CNTRL_EVENT_SEL_CNT_ARR_TIM2
#define TIMx_EVENT_TIM3 TIMER_CNTRL_EVENT_SEL_CNT_ARR_TIM3
#define TIMx_EVENT_CH1 TIMER_CNTRL_EVENT_SEL_CH1_MODE1
#define TIMx_EVENT_CH2 TIMER_CNTRL_EVENT_SEL_CH2_MODE1
#define TIMx_EVENT_CH3 TIMER_CNTRL_EVENT_SEL_CH3_MODE1
#define TIMx_EVENT_CH4 TIMER_CNTRL_EVENT_SEL_CH4_MODE1
#define TIMx_EVENT_ETR TIMER_CNTRL_EVENT_SEL_ETR_MODE2
/** @} */

/* ==================== PWM Channel Macros ==================== */
/**
 * @defgroup MIL_TIMx_Channel Timer Channel Selection
 * @{
 */
#define TIMx_CH1 0
#define TIMx_CH2 1
#define TIMx_CH3 2
#define TIMx_CH4 3
/** @} */

/* ==================== PWM Mode Macros ==================== */
/**
 * @defgroup PWM_Mode PWM Output Mode
 * @{
 */
#define PWM_MODE_FORCE_0 TIMER_CH_CNTRL_OCCM_REF_0
#define PWM_MODE_FORCE_1 TIMER_CH_CNTRL_OCCM_REF_1
#define PWM_MODE_SET TIMER_CH_CNTRL_OCCM_REF_1_CNT_CCR
#define PWM_MODE_CLR TIMER_CH_CNTRL_OCCM_REF_0_CNT_CCR
#define PWM_MODE_TOGGLE TIMER_CH_CNTRL_OCCM_SW_REF_CNT_CCR
#define PWM_MODE_UP TIMER_CH_CNTRL_OCCM_REF_1_DIR_0_CNT_CCR
#define PWM_MODE_DOWN TIMER_CH_CNTRL_OCCM_REF_1_DIR_1_CNT_CCR
/** @} */

/* ==================== Output Control Macros ==================== */
/**
 * @defgroup Output_Control PWM Output Control
 * @{
 */
#define OUTPUT_DISABLED TIMER_CH_CNTRL1_SELOE_OUT_DIS
#define OUTPUT_ENABLED TIMER_CH_CNTRL1_SELOE_OUT_EN
#define OUTPUT_REF TIMER_CH_CNTRL1_SELOE_OUT_REF_Z_OUT
#define OUTPUT_DTG TIMER_CH_CNTRL1_SELOE_OUT_DTG_Z_OUT
/** @} */

/* ==================== Interrupt Mask Macros ==================== */
/**
 * @defgroup MIL_TIMx_Interrupt Timer Interrupt Sources
 * @{
 */
#define TIMx_IT_CNT_ZERO TIMER_IE_CNT_ZERO_EVENT_IE
#define TIMx_IT_CNT_ARR TIMER_IE_CNT_ARR_EVENT_IE
#define TIMx_IT_ETR_RE TIMER_IE_ETR_RE_EVENT_IE
#define TIMx_IT_ETR_FE TIMER_IE_ETR_FE_EVENT_IE
#define TIMx_IT_BRK TIMER_IE_BRK_EVENT_IE
#define TIMx_IT_CCR1_CAP (1 << 5)
#define TIMx_IT_CCR2_CAP (1 << 6)
#define TIMx_IT_CCR3_CAP (1 << 7)
#define TIMx_IT_CCR4_CAP (1 << 8)
#define TIMx_IT_CCR1_REF (1 << 9)
#define TIMx_IT_CCR2_REF (1 << 10)
#define TIMx_IT_CCR3_REF (1 << 11)
#define TIMx_IT_CCR4_REF (1 << 12)
/** @} */

/* ==================== Structure Definitions ==================== */

/**
 * @brief Timer base configuration structure
 */
typedef struct {
  uint16_t prescaler;    /**< Timer prescaler value (0-65535) */
  uint16_t period;       /**< Auto-reload register value (0-65535) */
  uint16_t counter_mode; /**< Counter mode (up/down/up-down) */
  uint16_t event_source; /**< Event source selection */
  uint8_t clock_divider; /**< Clock divider for timer clock */
} MIL_TIMx_InitTypeDef;

/**
 * @brief PWM channel configuration structure
 */
typedef struct {
  uint8_t channel;              /**< Channel number (0-3) */
  uint16_t pulse;               /**< Pulse width value (0-65535) */
  uint16_t mode;                /**< PWM generation mode */
  uint8_t polarity;             /**< Output polarity (0=normal, 1=inverted) */
  uint8_t output_enable;        /**< Output enable mode */
  uint8_t complementary_enable; /**< Enable complementary output */
} PWM_ChannelTypeDef;

/**
 * @brief Dead-time generator configuration structure
 */
typedef struct {
  uint8_t channel;       /**< Channel number (0-3) */
  uint8_t dtg_prescaler; /**< Dead-time prescaler (0-15) */
  uint8_t dtg_value;     /**< Dead-time value (0-255) */
  uint8_t use_fdts;      /**< Use FDTS clock (0=TIM_CLK, 1=FDTS) */
} DeadTime_ConfigTypeDef;

/**
 * @brief Capture configuration structure
 */
typedef struct {
  uint8_t channel;     /**< Channel number (0-3) */
  uint8_t filter;      /**< Input filter configuration (0-15) */
  uint8_t edge_select; /**< Edge selection (0=rising, 1=falling, 2/3=other) */
  uint8_t prescaler;   /**< Input prescaler (0-3) */
} Capture_ConfigTypeDef;

/* ==================== Function Prototypes ==================== */

/* Clock and initialization functions */
void MIL_TIMx_EnableClock(MDR_TIMER_TypeDef* TIMERx, uint8_t clock_div);
void MIL_TIMx_DisableClock(MDR_TIMER_TypeDef* TIMERx);
void MIL_TIMx_Init(MDR_TIMER_TypeDef* TIMERx, MIL_TIMx_InitTypeDef* init);
void MIL_TIMx_DeInit(MDR_TIMER_TypeDef* TIMERx);

/* Counter control functions */
void MIL_TIMx_Start(MDR_TIMER_TypeDef* TIMERx);
void MIL_TIMx_Stop(MDR_TIMER_TypeDef* TIMERx);
void MIL_TIMx_SetCounter(MDR_TIMER_TypeDef* TIMERx, uint16_t value);
uint16_t MIL_TIMx_GetCounter(MDR_TIMER_TypeDef* TIMERx);
void MIL_TIMx_SetPeriod(MDR_TIMER_TypeDef* TIMERx, uint16_t period);
uint16_t MIL_TIMx_GetPeriod(MDR_TIMER_TypeDef* TIMERx);
void MIL_TIMx_SetPrescaler(MDR_TIMER_TypeDef* TIMERx, uint16_t prescaler);

/* PWM functions */
void MIL_TIMx_PWM_Init(MDR_TIMER_TypeDef* TIMERx, PWM_ChannelTypeDef* pwm_config);
void MIL_TIMx_PWM_SetDutyCycle(MDR_TIMER_TypeDef* TIMERx, uint8_t channel, uint16_t pulse);
void MIL_TIMx_PWM_SetDutyCyclePercent(MDR_TIMER_TypeDef* TIMERx, uint8_t channel, uint8_t percent);
void MIL_TIMx_PWM_Start(MDR_TIMER_TypeDef* TIMERx, uint8_t channel);
void MIL_TIMx_PWM_Stop(MDR_TIMER_TypeDef* TIMERx, uint8_t channel);

/* Dead-time functions */
void MIL_TIMx_DeadTime_Config(MDR_TIMER_TypeDef* TIMERx, DeadTime_ConfigTypeDef* dtg_config);
void MIL_TIMx_DeadTime_SetValue(MDR_TIMER_TypeDef* TIMERx, uint8_t channel, uint8_t dtg_value);
uint8_t MIL_TIMx_DeadTime_GetValue(MDR_TIMER_TypeDef* TIMERx, uint8_t channel);
/* Capture functions */
void MIL_TIMx_Capture_Init(MDR_TIMER_TypeDef* TIMERx, Capture_ConfigTypeDef* cap_config);
uint16_t MIL_TIMx_Capture_GetValue(MDR_TIMER_TypeDef* TIMERx, uint8_t channel);
uint16_t MIL_TIMx_Capture_GetValue1(MDR_TIMER_TypeDef* TIMERx, uint8_t channel);

/* Interrupt functions */
void MIL_TIMx_ITConfig(MDR_TIMER_TypeDef* TIMERx, uint32_t interrupt_mask, uint8_t enable);
uint32_t MIL_TIMx_GetITStatus(MDR_TIMER_TypeDef* TIMERx, uint32_t interrupt_mask);
void MIL_TIMx_ClearITPendingBit(MDR_TIMER_TypeDef* TIMERx, uint32_t interrupt_mask);

/* Break and ETR functions */
void MIL_TIMx_BRK_Config(MDR_TIMER_TypeDef* TIMERx, uint8_t polarity);
void MIL_TIMx_ETR_Config(MDR_TIMER_TypeDef* TIMERx, uint8_t polarity, uint8_t prescaler, uint8_t filter);
void MIL_TIMx_ETR_FilterConfig(MDR_TIMER_TypeDef* TIMERx, uint8_t filter);

#endif /* MIL_TIMX_H */
