/**
 * @file MIL_GPIO.c
 * @brief GPIO library implementation for Milandr 1986VE91T microcontroller
 * @author Mistress-Lukutar
 * @date 2026-07-29
 * @version v1.0.1
 */

#include "MIL_GPIO.h"
#include <stdlib.h>

/* Private function prototypes */
static uint32_t MIL_GPIO_GetPinPosition(uint16_t GPIO_Pin);

/**
 * @brief Initialize GPIO pins
 * @param GPIOx GPIO Port
 * @param GPIO_Init Pointer to GPIO initialization structure
 * @return None
 */
void MIL_GPIO_Init(MDR_PORT_TypeDef* GPIOx, MIL_GPIO_InitTypeDef* GPIO_Init) {
  uint32_t position = 0x00U;
  uint32_t temp_reg = 0x00U;
  uint32_t pin_mask = GPIO_Init->Pin;

  /* Check parameters */
  if (GPIOx == NULL || GPIO_Init == NULL)
    return;

  /* Configure GPIO clock */
  if (GPIOx == MDR_PORTA)
    MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_PORTA;
  else if (GPIOx == MDR_PORTB)
    MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_PORTB;
  else if (GPIOx == MDR_PORTC)
    MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_PORTC;
  else if (GPIOx == MDR_PORTD)
    MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_PORTD;
  else if (GPIOx == MDR_PORTE)
    MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_PORTE;
  else if (GPIOx == MDR_PORTF)
    MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_PORTF;
  else
    return; // Invalid GPIO pointer

  /* Configure each pin */
  while (pin_mask != 0U) {
    /* Get pin position */
    position = MIL_GPIO_GetPinPosition(pin_mask);

    if (position != 0xFFFFFFFFU) {
      /* Configure FUNC register */
      temp_reg = GPIOx->FUNC;
      temp_reg &= ~(PORT_FUNC_MODE_Msk << (position * 2U));
      temp_reg |= (GPIO_Init->FuncMode << (position * 2U));
      GPIOx->FUNC = temp_reg;

      /* Configure OE register (Output Enable) */
      if (GPIO_Init->Mode == MIL_GPIO_MODE_OUTPUT) {
        GPIOx->RXTX &= ~(1U << position);
        GPIOx->OE |= (1U << position);
      } else {
        GPIOx->OE &= ~(1U << position);
      }

      /* Configure ANALOG register */
      if (GPIO_Init->Mode == MIL_GPIO_MODE_ANALOG || GPIO_Init->AnalogMode != 0U) {
        GPIOx->ANALOG &= ~(1U << position);
      } else {
        GPIOx->ANALOG |= (1U << position);
      }

      /* Configure PULL register */
      GPIOx->PULL &= ~((1U << position) | (1U << (position + 16U)));
      if (GPIO_Init->Pull == MIL_GPIO_PULLDOWN) {
        GPIOx->PULL |= (1U << position);
      } else if (GPIO_Init->Pull == MIL_GPIO_PULLUP) {
        GPIOx->PULL |= (1U << (position + 16U));
      }

      /* Configure PD register (Driver Mode and Schmitt Trigger) */
      if (GPIO_Init->DriverMode == MIL_GPIO_DRIVER_OPEN_DRAIN) {
        GPIOx->PD |= (1U << position);
      } else {
        GPIOx->PD &= ~(1U << position);
      }

      if (GPIO_Init->SchmittMode == MIL_GPIO_SCHMITT_ENABLE) {
        GPIOx->PD |= (1U << (position + 16U));
      } else {
        GPIOx->PD &= ~(1U << (position + 16U));
      }

      /* Configure PWR register */
      temp_reg = GPIOx->PWR;
      temp_reg &= ~(PORT_PWR_Msk << (position * 2U));
      temp_reg |= (GPIO_Init->PowerMode << (position * 2U));
      GPIOx->PWR = temp_reg;

      /* Configure GFEN register (Input Filter) */
      if (GPIO_Init->FilterEnable != 0U) {
        GPIOx->GFEN |= (1U << position);
      } else {
        GPIOx->GFEN &= ~(1U << position);
      }
    }

    /* Clear processed bit */
    pin_mask &= ~(1U << position);
  }
}

/**
 * @brief Deinitialize GPIO pins to reset state
 * @param GPIOx GPIO Port
 * @param GPIO_Pin Pin mask to deinitialize
 * @return None
 */
void MIL_GPIO_DeInit(MDR_PORT_TypeDef* GPIOx, uint32_t GPIO_Pin) {
  uint32_t position = 0x00U;
  uint32_t pin_mask = GPIO_Pin;

  /* Check parameters */
  if (GPIOx == NULL)
    return;

  /* Reset each pin to default state */
  while (pin_mask != 0U) {
    position = MIL_GPIO_GetPinPosition(pin_mask);

    if (position != 0xFFFFFFFFU) {
      /* Reset to default values */
      GPIOx->FUNC &= ~(PORT_FUNC_MODE_Msk << (position * 2U));       /* PORT mode */
      GPIOx->OE &= ~(1U << position);                                /* Input */
      GPIOx->ANALOG &= ~(1U << position);                            /* Digital */
      GPIOx->PULL &= ~((1U << position) | (1U << (position + 16U))); /* No pull */
      GPIOx->PD &= ~((1U << position) | (1U << (position + 16U)));   /* Normal driver, Schmitt off */
      GPIOx->PWR &= ~(PORT_PWR_Msk << (position * 2U));              /* Power off */
      GPIOx->GFEN &= ~(1U << position);                              /* Filter off */
    }

    pin_mask &= ~(1U << position);
  }
}

/**
 * @brief Read specified input port pin
 * @param GPIOx GPIO Port
 * @param GPIO_Pin Pin number to read
 * @return Input pin value (MIL_GPIO_PIN_SET or MIL_GPIO_PIN_RESET)
 */
MIL_GPIO_PinState MIL_GPIO_ReadPin(MDR_PORT_TypeDef* GPIOx, uint16_t GPIO_Pin) {
  MIL_GPIO_PinState bitstatus = MIL_GPIO_PIN_RESET;

  /* Check parameters */
  if (GPIOx == NULL)
    return MIL_GPIO_PIN_RESET;

  if ((GPIOx->RXTX & GPIO_Pin) != 0U) {
    bitstatus = MIL_GPIO_PIN_SET;
  }

  return bitstatus;
}

/**
 * @brief Write to specified output port pin
 * @param GPIOx GPIO Port
 * @param GPIO_Pin Pin number to write
 * @param PinState Pin state to set (MIL_GPIO_PIN_SET or MIL_GPIO_PIN_RESET)
 * @return None
 */
void MIL_GPIO_WritePin(MDR_PORT_TypeDef* GPIOx, uint16_t GPIO_Pin, MIL_GPIO_PinState PinState) {
  /* Check parameters */
  if (GPIOx == NULL)
    return;

  if (PinState != MIL_GPIO_PIN_RESET) {
    GPIOx->RXTX |= GPIO_Pin;
  } else {
    GPIOx->RXTX &= ~((uint32_t)GPIO_Pin);
  }
}

/**
 * @brief Toggle specified output port pin
 * @param GPIOx GPIO Port
 * @param GPIO_Pin Pin number to toggle
 * @return None
 */
void MIL_GPIO_TogglePin(MDR_PORT_TypeDef* GPIOx, uint16_t GPIO_Pin) {
  /* Check parameters */
  if (GPIOx == NULL)
    return;

  GPIOx->RXTX ^= GPIO_Pin;
}

/**
 * @brief Set specified output port pins
 * @param GPIOx GPIO Port
 * @param GPIO_Pin Pin mask to set
 * @return None
 */
void MIL_GPIO_SetPins(MDR_PORT_TypeDef* GPIOx, uint32_t GPIO_Pin) {
  /* Check parameters */
  if (GPIOx == NULL)
    return;

  GPIOx->RXTX |= GPIO_Pin;
}

/**
 * @brief Reset specified output port pins
 * @param GPIOx GPIO Port
 * @param GPIO_Pin Pin mask to reset
 * @return None
 */
void MIL_GPIO_ResetPins(MDR_PORT_TypeDef* GPIOx, uint32_t GPIO_Pin) {
  /* Check parameters */
  if (GPIOx == NULL)
    return;

  GPIOx->RXTX &= ~GPIO_Pin;
}

/**
 * @brief Read entire port
 * @param GPIOx GPIO Port
 * @return Port value
 */
uint16_t MIL_GPIO_ReadPort(MDR_PORT_TypeDef* GPIOx) {
  /* Check parameters */
  if (GPIOx == NULL)
    return 0U;

  return (uint16_t)(GPIOx->RXTX & 0xFFFFU);
}

/**
 * @brief Write to entire port
 * @param GPIOx GPIO Port
 * @param PortVal Value to write to port
 * @return None
 */
void MIL_GPIO_WritePort(MDR_PORT_TypeDef* GPIOx, uint16_t PortVal) {
  /* Check parameters */
  if (GPIOx == NULL)
    return;

  GPIOx->RXTX = (uint32_t)PortVal;
}

/**
 * @brief Get pin position from pin mask
 * @param GPIO_Pin Pin mask
 * @return Pin position (0-15) or 0xFFFFFFFF if invalid
 */
static uint32_t MIL_GPIO_GetPinPosition(uint16_t GPIO_Pin) {
  uint32_t position = 0U;

  /* Find first set bit */
  while (position < 16U) {
    if ((GPIO_Pin & (1U << position)) != 0U) {
      return position;
    }
    position++;
  }

  return 0xFFFFFFFFU; /* Invalid pin */
}
