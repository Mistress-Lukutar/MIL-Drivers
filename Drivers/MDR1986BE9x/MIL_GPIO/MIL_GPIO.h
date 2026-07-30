/**
 * @file MIL_GPIO.h
 * @brief GPIO library for Milandr 1986VE91T microcontroller
 * @author Mistress-Lukutar
 * @date 2026-07-30
 * @version v1.0.1
 */

#ifndef MIL_GPIO_H
#define MIL_GPIO_H

#include "MDR32Fx.h"
#include "stdint.h"

/**
 * @brief GPIO Pin Numbers
 */
#define MIL_GPIO_PIN_0 0x0001U   /*!< Pin 0 selected */
#define MIL_GPIO_PIN_1 0x0002U   /*!< Pin 1 selected */
#define MIL_GPIO_PIN_2 0x0004U   /*!< Pin 2 selected */
#define MIL_GPIO_PIN_3 0x0008U   /*!< Pin 3 selected */
#define MIL_GPIO_PIN_4 0x0010U   /*!< Pin 4 selected */
#define MIL_GPIO_PIN_5 0x0020U   /*!< Pin 5 selected */
#define MIL_GPIO_PIN_6 0x0040U   /*!< Pin 6 selected */
#define MIL_GPIO_PIN_7 0x0080U   /*!< Pin 7 selected */
#define MIL_GPIO_PIN_8 0x0100U   /*!< Pin 8 selected */
#define MIL_GPIO_PIN_9 0x0200U   /*!< Pin 9 selected */
#define MIL_GPIO_PIN_10 0x0400U  /*!< Pin 10 selected */
#define MIL_GPIO_PIN_11 0x0800U  /*!< Pin 11 selected */
#define MIL_GPIO_PIN_12 0x1000U  /*!< Pin 12 selected */
#define MIL_GPIO_PIN_13 0x2000U  /*!< Pin 13 selected */
#define MIL_GPIO_PIN_14 0x4000U  /*!< Pin 14 selected */
#define MIL_GPIO_PIN_15 0x8000U  /*!< Pin 15 selected */
#define MIL_GPIO_PIN_ALL 0xFFFFU /*!< All pins selected */

/**
 * @brief GPIO pin states
 */
typedef enum {
  MIL_GPIO_PIN_RESET = 0U, /*!< GPIO pin state reset */
  MIL_GPIO_PIN_SET   = 1U  /*!< GPIO pin state set */
} MIL_GPIO_PinState;

/**
 * @brief GPIO Mode enumeration
 */
typedef enum {
  MIL_GPIO_MODE_INPUT  = 0x00U, /*!< Input Floating Mode */
  MIL_GPIO_MODE_OUTPUT = 0x01U, /*!< Output Mode */
  MIL_GPIO_MODE_AF     = 0x02U, /*!< Alternate function Mode */
  MIL_GPIO_MODE_ANALOG = 0x03U  /*!< Analog Mode */
} MIL_GPIO_Mode;

/**
 * @brief GPIO Function Mode enumeration
 */
typedef enum {
  MIL_GPIO_FUNC_PORT = 0U, /*!< Port function */
  MIL_GPIO_FUNC_MAIN = 1U, /*!< Main function */
  MIL_GPIO_FUNC_ALT  = 2U, /*!< Alternate function */
  MIL_GPIO_FUNC_OVER = 3U  /*!< Override function */
} MIL_GPIO_FuncMode;

/**
 * @brief GPIO Pull-Up/Pull-Down enumeration
 */
typedef enum {
  MIL_GPIO_NOPULL   = 0x00U, /*!< No Pull-up or Pull-down activation */
  MIL_GPIO_PULLDOWN = 0x01U, /*!< Pull-down activation */
  MIL_GPIO_PULLUP   = 0x02U  /*!< Pull-up activation */
} MIL_GPIO_Pull;

/**
 * @brief GPIO Output Driver Mode enumeration
 */
typedef enum {
  MIL_GPIO_DRIVER_NORMAL     = 0U, /*!< Normal driver mode */
  MIL_GPIO_DRIVER_OPEN_DRAIN = 1U  /*!< Open drain driver mode */
} MIL_GPIO_DriverMode;

/**
 * @brief GPIO Power Mode enumeration
 */
typedef enum {
  MIL_GPIO_POWER_OFF      = 0U, /*!< Power off */
  MIL_GPIO_POWER_SLOW     = 1U, /*!< Slow power */
  MIL_GPIO_POWER_FAST     = 2U, /*!< Fast power */
  MIL_GPIO_POWER_MAX_FAST = 3U  /*!< Maximum fast power */
} MIL_GPIO_PowerMode;

/**
 * @brief GPIO Schmitt Trigger Mode enumeration
 */
typedef enum {
  MIL_GPIO_SCHMITT_DISABLE = 0U, /*!< Schmitt trigger disabled (200mV hysteresis) */
  MIL_GPIO_SCHMITT_ENABLE  = 1U  /*!< Schmitt trigger enabled (400mV hysteresis) */
} MIL_GPIO_SchmittMode;

/**
 * @brief GPIO Configuration Structure
 */
typedef struct {
  uint32_t Pin;                     /*!< Specifies the GPIO pins to be configured */
  MIL_GPIO_Mode Mode;               /*!< Specifies the operating mode for the selected pins */
  MIL_GPIO_FuncMode FuncMode;       /*!< Specifies the function mode for the selected pins */
  MIL_GPIO_Pull Pull;               /*!< Specifies the Pull-up or Pull-Down activation */
  MIL_GPIO_DriverMode DriverMode;   /*!< Specifies the driver mode for the selected pins */
  MIL_GPIO_PowerMode PowerMode;     /*!< Specifies the power mode for the selected pins */
  MIL_GPIO_SchmittMode SchmittMode; /*!< Specifies the Schmitt trigger mode */
  uint32_t AnalogMode;              /*!< Specifies analog mode (1 = analog, 0 = digital) */
  uint32_t FilterEnable;            /*!< Specifies input filter enable (1 = enabled) */
} MIL_GPIO_InitTypeDef;

/* Function Prototypes */

/**
 * @brief Initialize GPIO pins
 * @param GPIOx GPIO Port
 * @param GPIO_Init Pointer to GPIO initialization structure
 * @return None
 */
void MIL_GPIO_Init(MDR_PORT_TypeDef* GPIOx, MIL_GPIO_InitTypeDef* GPIO_Init);

/**
 * @brief Deinitialize GPIO pins to reset state
 * @param GPIOx GPIO Port
 * @param GPIO_Pin Pin mask to deinitialize
 * @return None
 */
void MIL_GPIO_DeInit(MDR_PORT_TypeDef* GPIOx, uint32_t GPIO_Pin);

/**
 * @brief Read specified input port pin
 * @param GPIOx GPIO Port
 * @param GPIO_Pin Pin number to read
 * @return Input pin value (MIL_GPIO_PIN_SET or MIL_GPIO_PIN_RESET)
 */
MIL_GPIO_PinState MIL_GPIO_ReadPin(MDR_PORT_TypeDef* GPIOx, uint16_t GPIO_Pin);

/**
 * @brief Write to specified output port pin
 * @param GPIOx GPIO Port
 * @param GPIO_Pin Pin number to write
 * @param PinState Pin state to set (MIL_GPIO_PIN_SET or MIL_GPIO_PIN_RESET)
 * @return None
 */
void MIL_GPIO_WritePin(MDR_PORT_TypeDef* GPIOx, uint16_t GPIO_Pin, MIL_GPIO_PinState PinState);

/**
 * @brief Toggle specified output port pin
 * @param GPIOx GPIO Port
 * @param GPIO_Pin Pin number to toggle
 * @return None
 */
void MIL_GPIO_TogglePin(MDR_PORT_TypeDef* GPIOx, uint16_t GPIO_Pin);

/**
 * @brief Set specified output port pins
 * @param GPIOx GPIO Port
 * @param GPIO_Pin Pin mask to set
 * @return None
 */
void MIL_GPIO_SetPins(MDR_PORT_TypeDef* GPIOx, uint32_t GPIO_Pin);

/**
 * @brief Reset specified output port pins
 * @param GPIOx GPIO Port
 * @param GPIO_Pin Pin mask to reset
 * @return None
 */
void MIL_GPIO_ResetPins(MDR_PORT_TypeDef* GPIOx, uint32_t GPIO_Pin);

/**
 * @brief Read entire port
 * @param GPIOx GPIO Port
 * @return Port value
 */
uint16_t MIL_GPIO_ReadPort(MDR_PORT_TypeDef* GPIOx);

/**
 * @brief Write to entire port
 * @param GPIOx GPIO Port
 * @param PortVal Value to write to port
 * @return None
 */
void MIL_GPIO_WritePort(MDR_PORT_TypeDef* GPIOx, uint16_t PortVal);

#endif /* MIL_GPIO_H */
