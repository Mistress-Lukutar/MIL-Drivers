/**
 * @file MIL_Uart.c
 * @brief Universal UART driver implementation
 *
 * Supports both UART1 and UART2 with configurable GPIO pins.
 * Provides half-duplex and full-duplex modes with blocking and
 * interrupt-based transmission/reception.
 *
 * @author Mistress-Lukutar
 * @date   2026-07-30
 * @version v2.1.0
 */

#include "MIL_Uart.h"
#include "MIL_Time.h"
#include <string.h>

/* ======================== Private Variables ======================== */

static MIL_UART_HandleTypeDef* uartHandles[2] = { NULL, NULL };

/* ======================== Private Functions ======================== */

/**
 * @brief Configure GPIO pin function and direction
 *
 * @param port GPIO port pointer
 * @param pin Pin number (0-15)
 * @param func Function mode (0=Port, 1=Main, 2=Alt, 3=Over)
 * @param output 1 for output, 0 for input
 */
static void _configPin(MDR_PORT_TypeDef* port, uint8_t pin, uint8_t func, uint8_t output) {
  /* Set pin function */
  uint32_t funcReg = port->FUNC;
  funcReg &= ~(PORT_FUNC_MODE_Msk << (pin * 2));
  funcReg |= (func & PORT_FUNC_MODE_Msk) << (pin * 2);
  port->FUNC = funcReg;

  /* Set pin direction */
  if (output) {
    port->OE |= (1U << pin);
  } else {
    port->OE &= ~(1U << pin);
  }
}

/**
 * @brief Switch UART direction for half-duplex operation
 *
 * Reconfigures TX and RX pins depending on desired direction.
 * In RX mode: RX pin is configured as UART function, TX as GPIO input.
 * In TX mode: TX pin is configured as UART function, RX as GPIO input.
 *
 * @param uart Pointer to UART handle
 * @param direction MIL_UART_DIRECTION_RX or MIL_UART_DIRECTION_TX
 */
static void _switchDirection(MIL_UART_HandleTypeDef* uart, uint8_t direction) {
  if (direction == MIL_UART_DIRECTION_TX) {
    /* Configure TX pin for UART function (output) */
    _configPin(uart->txPin.port, uart->txPin.pin, PORT_FUNC_MODE_OVER, 1);

    /* Configure RX pin as GPIO input (unused) */
    _configPin(uart->rxPin.port, uart->rxPin.pin, PORT_FUNC_MODE_PORT, 0);
  } else {
    /* Configure RX pin for UART function (input) */
    _configPin(uart->rxPin.port, uart->rxPin.pin, PORT_FUNC_MODE_OVER, 0);

    /* Configure TX pin as GPIO input (unused) */
    _configPin(uart->txPin.port, uart->txPin.pin, PORT_FUNC_MODE_PORT, 0);
  }
}

/**
 * @brief Get UART instance index (0 for UART1, 1 for UART2)
 *
 * @param uart Pointer to UART handle
 * @return Instance index or 0xFF if invalid
 */
static uint8_t _getInstanceIndex(const MIL_UART_HandleTypeDef* uart) {
  if (uart->instance == MDR_UART1) {
    return 0;
  } else if (uart->instance == MDR_UART2) {
    return 1;
  }
  return 0xFF;
}

/**
 * @brief Enable UART peripheral clock
 *
 * @param uart Pointer to UART handle
 */
static void _enableClock(MIL_UART_HandleTypeDef* uart) {
  if (uart->instance == MDR_UART1) {
    MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_UART1;
    MDR_RST_CLK->UART_CLOCK |= RST_CLK_UART_CLOCK_UART1_CLK_EN | (uart->clkDiv & 0xFF);
  } else if (uart->instance == MDR_UART2) {
    MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_UART2;
    MDR_RST_CLK->UART_CLOCK |= RST_CLK_UART_CLOCK_UART2_CLK_EN | ((uart->clkDiv & 0xFF) << 8);
  }
}

/**
 * @brief Configure GPIO pins for UART operation
 *
 * Sets pins to digital mode, fast speed, and initial RX direction.
 *
 * @param uart Pointer to UART handle
 */
static void _configureGPIO(MIL_UART_HandleTypeDef* uart) {
  /* Enable digital mode for TX and RX pins */
  uart->txPin.port->ANALOG |= (1U << uart->txPin.pin);
  uart->rxPin.port->ANALOG |= (1U << uart->rxPin.pin);

  /* Set fast speed for both pins */
  uint32_t pwrReg = uart->txPin.port->PWR;
  pwrReg &= ~(PORT_PWR_Msk << (uart->txPin.pin * 2));
  pwrReg |= (PORT_PWR_FAST << (uart->txPin.pin * 2));
  uart->txPin.port->PWR = pwrReg;

  pwrReg = uart->rxPin.port->PWR;
  pwrReg &= ~(PORT_PWR_Msk << (uart->rxPin.pin * 2));
  pwrReg |= (PORT_PWR_FAST << (uart->rxPin.pin * 2));
  uart->rxPin.port->PWR = pwrReg;

  if (uart->mode == MIL_UART_MODE_FULL_DUPLEX) {
    /* Full-duplex: both TX and RX pins are UART function simultaneously */
    _configPin(uart->txPin.port, uart->txPin.pin, PORT_FUNC_MODE_OVER, 1);
    _configPin(uart->rxPin.port, uart->rxPin.pin, PORT_FUNC_MODE_OVER, 0);
  } else {
    /* Half-duplex: initial direction is RX */
    _switchDirection(uart, MIL_UART_DIRECTION_RX);
  }
}

/* ======================== Public Functions ======================== */

MIL_UART_ErrorTypeDef MIL_UART_Init(MIL_UART_HandleTypeDef* uart) {
  if (uart == NULL || uart->instance == NULL) {
    return MIL_UART_ERROR_INVALID_PARAM;
  }

  /* Configure GPIO pins */
  _configureGPIO(uart);

  /* Enable UART peripheral clock */
  _enableClock(uart);

  /* Disable UART before configuration */
  uart->instance->CR = 0;

  /* Set baud rate divisors */
  uart->instance->IBRD = uart->ibrd;
  uart->instance->FBRD = uart->fbrd;

  /* Configure line control: FIFO enabled, 8-bit data, no parity, 1 stop bit
   */
  uart->instance->LCR_H = UART_LCR_H_FEN |             /* Enable FIFO */
      (UART_LCR_H_WLEN_8_BITS << UART_LCR_H_WLEN_Pos); /* 8-bit word length */

  /* Enable UART: RX enabled, TX enabled, UART enabled */
  uart->instance->CR = UART_CR_RXE | /* Enable receiver */
      UART_CR_TXE |                  /* Enable transmitter */
      UART_CR_UARTEN;                /* Enable UART */

  /* Clear all interrupt flags */
  uart->instance->ICR = 0x7FF;

  /* Initialize buffer indices */
  uart->txHead = 0;
  uart->txTail = 0;
  uart->rxHead = 0;
  uart->rxTail = 0;

  /* Set initial status */
  uart->status = MIL_UART_READY;

  return MIL_UART_OK;
}

MIL_UART_ErrorTypeDef MIL_UART_InitIT(MIL_UART_HandleTypeDef* uart) {
  MIL_UART_ErrorTypeDef result = MIL_UART_Init(uart);
  if (result != MIL_UART_OK) {
    return result;
  }

  /* Store handle in global array for IRQ handler */
  uint8_t idx = _getInstanceIndex(uart);
  if (idx >= 2) {
    return MIL_UART_ERROR_INVALID_PARAM;
  }
  uartHandles[idx] = uart;

  /* Disable FIFO for byte-by-byte interrupt operation */
  uart->instance->LCR_H &= ~UART_LCR_H_FEN;

  /* Clear any latched interrupt flags BEFORE unmasking them, so a flag that
   * fired between the two writes cannot trigger a spurious IRQ. ICR is
   * write-1-to-clear, so write the mask directly (not a read-modify-write). */
  uart->instance->ICR = UART_ICR_TXIC | UART_ICR_RXIC;
  __DSB();
  uart->instance->IMSC = UART_IMSC_TXIM | UART_IMSC_RXIM;

  /* Enable UART interrupt in NVIC. Drain the write buffer first so IMSC is
   * committed before the NVIC line goes live. */
  __DSB();
  if (uart->instance == MDR_UART1) {
    NVIC_EnableIRQ(UART1_IRQn);
  } else if (uart->instance == MDR_UART2) {
    NVIC_EnableIRQ(UART2_IRQn);
  }

  return MIL_UART_OK;
}

MIL_UART_ErrorTypeDef MIL_UART_Send(MIL_UART_HandleTypeDef* uart, const uint8_t* data, uint16_t length, uint32_t timeout) {
  if (uart == NULL || data == NULL || length == 0) {
    return MIL_UART_ERROR_INVALID_PARAM;
  }

  uint32_t startTime = MIL_TIME_GetMillis();

  if (uart->mode == MIL_UART_MODE_HALF_DUPLEX) {
    _switchDirection(uart, MIL_UART_DIRECTION_TX);
  }

  /* Transmit each byte */
  for (uint16_t i = 0; i < length; i++) {
    /* Wait until TX FIFO is not full */
    while (uart->instance->FR & UART_FR_TXFF) {
      if ((MIL_TIME_GetMillis() - startTime) > timeout) {
        if (uart->mode == MIL_UART_MODE_HALF_DUPLEX) {
          _switchDirection(uart, MIL_UART_DIRECTION_RX);
        }
        return MIL_UART_ERROR_TIMEOUT;
      }
    }
    uart->instance->DR = data[i];
  }

  /* Wait until transmission complete (BUSY flag clear) */
  while (uart->instance->FR & UART_FR_BUSY) {
    if ((MIL_TIME_GetMillis() - startTime) > timeout) {
      if (uart->mode == MIL_UART_MODE_HALF_DUPLEX) {
        _switchDirection(uart, MIL_UART_DIRECTION_RX);
      }
      return MIL_UART_ERROR_TIMEOUT;
    }
  }

  if (uart->mode == MIL_UART_MODE_HALF_DUPLEX) {
    _switchDirection(uart, MIL_UART_DIRECTION_RX);
  }
  return MIL_UART_OK;
}

MIL_UART_ErrorTypeDef MIL_UART_SendString(MIL_UART_HandleTypeDef* uart, const char* str, uint32_t timeout) {
  if (str == NULL) {
    return MIL_UART_ERROR_INVALID_PARAM;
  }
  return MIL_UART_Send(uart, (const uint8_t*)str, strlen(str), timeout);
}

MIL_UART_ErrorTypeDef MIL_UART_SendIT(MIL_UART_HandleTypeDef* uart, const uint8_t* data, uint16_t length) {
  if (uart == NULL || data == NULL || length == 0) {
    return MIL_UART_ERROR_INVALID_PARAM;
  }

  /* Copy data to transmit buffer */
  for (uint16_t i = 0; i < length; i++) {
    uint16_t nextHead = (uart->txHead + 1) % UART_TX_BUFFER_SIZE;

    /* Check for buffer overflow */
    if (nextHead == uart->txTail) {
      return MIL_UART_ERROR_BUFFER_FULL;
    }

    uart->txBuffer[uart->txHead] = data[i];
    uart->txHead                 = nextHead;
  }

  /* Start transmission if not already in progress */
  if (uart->status == MIL_UART_READY) {
    uart->status = MIL_UART_BUSY;
    if (uart->mode == MIL_UART_MODE_HALF_DUPLEX) {
      _switchDirection(uart, MIL_UART_DIRECTION_TX);
    }

    /* Trigger first byte transmission */
    uart->instance->DR = uart->txBuffer[uart->txTail];
    uart->txTail       = (uart->txTail + 1) % UART_TX_BUFFER_SIZE;
  }

  return MIL_UART_OK;
}

MIL_UART_ErrorTypeDef MIL_UART_SendStringIT(MIL_UART_HandleTypeDef* uart, const char* str) {
  if (str == NULL) {
    return MIL_UART_ERROR_INVALID_PARAM;
  }
  return MIL_UART_SendIT(uart, (const uint8_t*)str, strlen(str));
}

MIL_UART_ErrorTypeDef MIL_UART_Receive(MIL_UART_HandleTypeDef* uart, uint8_t* data, uint16_t length, uint32_t timeout) {
  if (uart == NULL || data == NULL || length == 0) {
    return MIL_UART_ERROR_INVALID_PARAM;
  }

  uint32_t startTime = MIL_TIME_GetMillis();

  /* Receive each byte */
  for (uint16_t i = 0; i < length; i++) {
    /* Wait until RX FIFO is not empty */
    while (uart->instance->FR & UART_FR_RXFE) {
      if ((MIL_TIME_GetMillis() - startTime) > timeout) {
        return MIL_UART_ERROR_TIMEOUT;
      }
    }
    data[i] = (uint8_t)(uart->instance->DR & 0xFF);
  }

  return MIL_UART_OK;
}

uint16_t MIL_UART_GetRxAvailable(const MIL_UART_HandleTypeDef* uart) {
  if (uart == NULL) {
    return 0;
  }

  if (uart->rxHead >= uart->rxTail) {
    return uart->rxHead - uart->rxTail;
  } else {
    return UART_RX_BUFFER_SIZE - uart->rxTail + uart->rxHead;
  }
}

uint16_t MIL_UART_GetTxFree(const MIL_UART_HandleTypeDef* uart) {
  if (uart == NULL) {
    return 0;
  }

  if (uart->txHead >= uart->txTail) {
    return UART_TX_BUFFER_SIZE - (uart->txHead - uart->txTail) - 1U;
  } else {
    return uart->txTail - uart->txHead - 1U;
  }
}

MIL_UART_ErrorTypeDef MIL_UART_ReadByte(MIL_UART_HandleTypeDef* uart, uint8_t* data) {
  if (uart == NULL || data == NULL) {
    return MIL_UART_ERROR_INVALID_PARAM;
  }

  /* Check if data available */
  if (uart->rxHead == uart->rxTail) {
    return MIL_UART_ERROR_TIMEOUT; /* No data available */
  }

  *data        = uart->rxBuffer[uart->rxTail];
  uart->rxTail = (uart->rxTail + 1) % UART_RX_BUFFER_SIZE;

  return MIL_UART_OK;
}

void MIL_UART_IRQHandler(MIL_UART_HandleTypeDef* uart) {
  if (uart == NULL) {
    return;
  }

  /* TX interrupt - transmit next byte */
  if (uart->instance->RIS & UART_RIS_TXRIS) {
    uart->instance->ICR = UART_ICR_TXIC; /* Clear TX interrupt (W1C: direct write, not RMW) */

    if (uart->txHead != uart->txTail) {
      /* More data to send: write from buffer then advance tail */
      uart->instance->DR = uart->txBuffer[uart->txTail];
      uart->txTail       = (uart->txTail + 1) % UART_TX_BUFFER_SIZE;
    } else {
      /* Transmission complete */
      uart->status = MIL_UART_READY;
      if (uart->mode == MIL_UART_MODE_HALF_DUPLEX) {
        _switchDirection(uart, MIL_UART_DIRECTION_RX);
      }
    }
  }

  /* RX interrupt - receive byte */
  if (uart->instance->RIS & UART_RIS_RXRIS) {
    uart->instance->ICR = UART_ICR_RXIC; /* Clear RX interrupt (W1C: direct write, not RMW) */

    uint16_t nextHead = (uart->rxHead + 1) % UART_RX_BUFFER_SIZE;

    /* Store received byte if buffer not full */
    if (nextHead != uart->rxTail) {
      uart->rxBuffer[uart->rxHead] = (uint8_t)(uart->instance->DR & 0xFF);
      uart->rxHead                 = nextHead;
    } else {
      /* Buffer overflow - discard byte */
      (void)uart->instance->DR;
    }
  }
}

/* ======================== IRQ Handler Wrappers ======================== */

/**
 * @brief UART1 interrupt handler
 */
void UART1_IRQHandler(void) {
  if (uartHandles[0] != NULL) {
    MIL_UART_IRQHandler(uartHandles[0]);
  }
}

/**
 * @brief UART2 interrupt handler
 */
void UART2_IRQHandler(void) {
  if (uartHandles[1] != NULL) {
    MIL_UART_IRQHandler(uartHandles[1]);
  }
}
