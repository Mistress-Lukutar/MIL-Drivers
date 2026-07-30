/**
 * @file MIL_SPI.c
 * @brief SPI/SSP driver implementation for 1986BE9x series MCUs
 * @author Mistress-Lukutar
 * @date   2026-05-18
 * @version v1.0.0
 */

#include "MIL_SPI.h"
#include "MIL_Time.h"

/* Private function prototypes */
static void _enableClock(MIL_SPI_HandleTypeDef* hspi);
static uint8_t _waitForTxNotFull(MIL_SPI_HandleTypeDef* hspi, uint32_t timeout);
static uint8_t _waitForRxNotEmpty(MIL_SPI_HandleTypeDef* hspi, uint32_t timeout);
static uint8_t _waitForNotBusy(MIL_SPI_HandleTypeDef* hspi, uint32_t timeout);

/**
 * @brief Enable SSP peripheral clock
 * @param hspi Pointer to SPI handle
 */
static void _enableClock(MIL_SPI_HandleTypeDef* hspi) {
  if (hspi->Instance == MDR_SSP1) {
    MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_SPI1;
    MDR_RST_CLK->SSP_CLOCK |= RST_CLK_SSP_CLOCK_SSP1_CLK_EN;
    /* Set clock divider - default HCLK (no division) */
    MDR_RST_CLK->SSP_CLOCK &= ~RST_CLK_SSP_CLOCK_SSP1_BRG_Msk;
    MDR_RST_CLK->SSP_CLOCK |= RST_CLK_SSP_CLOCK_SSP_BRG_HCLK << RST_CLK_SSP_CLOCK_SSP1_BRG_Pos;
  } else if (hspi->Instance == MDR_SSP2) {
    MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_SPI2;
    MDR_RST_CLK->SSP_CLOCK |= RST_CLK_SSP_CLOCK_SSP2_CLK_EN;
    /* Set clock divider - default HCLK (no division) */
    MDR_RST_CLK->SSP_CLOCK &= ~RST_CLK_SSP_CLOCK_SSP2_BRG_Msk;
    MDR_RST_CLK->SSP_CLOCK |= RST_CLK_SSP_CLOCK_SSP_BRG_HCLK << RST_CLK_SSP_CLOCK_SSP2_BRG_Pos;
  }
}

/**
 * @brief Wait for TX FIFO not full with timeout
 * @param hspi Pointer to SPI handle
 * @param timeout Timeout in milliseconds
 * @return 1 if success, 0 if timeout
 */
static uint8_t _waitForTxNotFull(MIL_SPI_HandleTypeDef* hspi, uint32_t timeout) {
  uint32_t start = MIL_TIME_GetMillis();
  while (!(hspi->Instance->SR & SSP_SR_TNF)) {
    if (MIL_TIME_GetMillis() - start >= timeout) {
      return 0U;
    }
  }
  return 1U;
}

/**
 * @brief Wait for RX FIFO not empty with timeout
 * @param hspi Pointer to SPI handle
 * @param timeout Timeout in milliseconds
 * @return 1 if success, 0 if timeout
 */
static uint8_t _waitForRxNotEmpty(MIL_SPI_HandleTypeDef* hspi, uint32_t timeout) {
  uint32_t start = MIL_TIME_GetMillis();
  while (!(hspi->Instance->SR & SSP_SR_RNE)) {
    if (MIL_TIME_GetMillis() - start >= timeout) {
      return 0U;
    }
  }
  return 1U;
}

/**
 * @brief Wait for SPI not busy with timeout
 * @param hspi Pointer to SPI handle
 * @param timeout Timeout in milliseconds
 * @return 1 if success, 0 if timeout
 */
static uint8_t _waitForNotBusy(MIL_SPI_HandleTypeDef* hspi, uint32_t timeout) {
  uint32_t start = MIL_TIME_GetMillis();
  while (hspi->Instance->SR & SSP_SR_BSY) {
    if (MIL_TIME_GetMillis() - start >= timeout) {
      return 0U;
    }
  }
  return 1U;
}

/**
 * @brief Initialize SPI peripheral
 * @param hspi Pointer to SPI handle
 * @return MIL_SPI_State Current state
 */
MIL_SPI_State MIL_SPI_Init(MIL_SPI_HandleTypeDef* hspi) {
  uint32_t cr0;

  if (hspi == NULL) {
    return MIL_SPI_STATE_ERROR;
  }

  if (hspi->Instance == NULL) {
    return MIL_SPI_STATE_ERROR;
  }

  /* Enable peripheral clock */
  _enableClock(hspi);

  /* Disable SPI before configuration */
  MIL_SPI_Disable(hspi);

  /* Configure CR0 register */
  cr0 = 0U;
  /* Data size */
  cr0 |= (hspi->DataSize & SSP_CR0_DSS_Msk);
  /* Protocol */
  cr0 |= ((hspi->Protocol << SSP_CR0_FRF_Pos) & SSP_CR0_FRF_Msk);
  /* Clock polarity and phase (for SPI mode) */
  cr0 |= (hspi->ModeType & (SSP_CR0_SPO | SSP_CR0_SPH));
  /* Clock rate divider */
  cr0 |= (((uint32_t)hspi->Divider << SSP_CR0_SCR_Pos) & SSP_CR0_SCR_Msk);

  hspi->Instance->CR0 = cr0;

  /* Configure prescaler */
  hspi->Instance->CPSR = (uint32_t)hspi->Prescaler & SSP_CPSR_CPSDVSR_Msk;

  /* Configure CR1 register */
  if (hspi->Mode == MIL_SPI_MODE_SLAVE) {
    hspi->Instance->CR1 |= SSP_CR1_MS;
  } else {
    hspi->Instance->CR1 &= ~SSP_CR1_MS;
  }

  /* Clear loopback mode */
  hspi->Instance->CR1 &= ~SSP_CR1_LBM;

  /* Flush RX FIFO while disabled — may contain stale data from reset */
  while (hspi->Instance->SR & SSP_SR_RNE) {
    (void)hspi->Instance->DR;
  }

  /* Clear any stale interrupt/error flags */
  hspi->Instance->ICR = SSP_ICR_RORIC | SSP_ICR_RTIC;

  /* Enable SPI */
  MIL_SPI_Enable(hspi);

  hspi->State     = MIL_SPI_STATE_READY;
  hspi->ErrorCode = MIL_SPI_ERROR_NONE;

  return hspi->State;
}

/**
 * @brief Deinitialize SPI peripheral
 * @param hspi Pointer to SPI handle
 * @return MIL_SPI_State Current state
 */
MIL_SPI_State MIL_SPI_DeInit(MIL_SPI_HandleTypeDef* hspi) {
  if (hspi == NULL) {
    return MIL_SPI_STATE_ERROR;
  }

  /* Disable SPI */
  MIL_SPI_Disable(hspi);

  /* Disable clock */
  if (hspi->Instance == MDR_SSP1) {
    MDR_RST_CLK->SSP_CLOCK &= ~RST_CLK_SSP_CLOCK_SSP1_CLK_EN;
  } else if (hspi->Instance == MDR_SSP2) {
    MDR_RST_CLK->SSP_CLOCK &= ~RST_CLK_SSP_CLOCK_SSP2_CLK_EN;
  }

  hspi->State = MIL_SPI_STATE_RESET;

  return MIL_SPI_STATE_RESET;
}

/**
 * @brief Initialize GPIO for SPI (CLK, RXD, TXD)
 * @param hspi Pointer to SPI handle
 * @param port GPIO port for SPI pins
 */
void MIL_SPI_GPIO_Init(MIL_SPI_HandleTypeDef* hspi, MDR_PORT_TypeDef* port) {
  MIL_GPIO_InitTypeDef gpioInit;

  if (hspi == NULL || port == NULL) {
    return;
  }

  /* Common settings for SPI pins */
  gpioInit.Mode       = MIL_GPIO_MODE_AF;
  gpioInit.FuncMode   = MIL_GPIO_FUNC_ALT;
  gpioInit.PowerMode  = MIL_GPIO_POWER_MAX_FAST;
  gpioInit.Pull       = MIL_GPIO_NOPULL;
  gpioInit.DriverMode = MIL_GPIO_DRIVER_NORMAL;

  if (hspi->Instance == MDR_SSP1) {
    /* SSP1 on Port F: PF0=TXD, PF1=CLK, PF2=FSS, PF3=RXD */
    /* TXD - PF0 */
    gpioInit.Pin = MIL_GPIO_PIN_0;
    MIL_GPIO_Init(port, &gpioInit);
    /* CLK - PF1 */
    gpioInit.Pin = MIL_GPIO_PIN_1;
    MIL_GPIO_Init(port, &gpioInit);
    /* RXD - PF3 */
    gpioInit.Pin = MIL_GPIO_PIN_3;
    MIL_GPIO_Init(port, &gpioInit);
  } else if (hspi->Instance == MDR_SSP2) {
    /* SSP2 on Port D: PD2=RXD, PD3=FSS, PD5=CLK, PD6=TXD */
    /* RXD - PD2 */
    gpioInit.Pin = MIL_GPIO_PIN_2;
    MIL_GPIO_Init(port, &gpioInit);
    /* CLK - PD5 */
    gpioInit.Pin = MIL_GPIO_PIN_5;
    MIL_GPIO_Init(port, &gpioInit);
    /* TXD - PD6 */
    gpioInit.Pin = MIL_GPIO_PIN_6;
    MIL_GPIO_Init(port, &gpioInit);
  }
}

/**
 * @brief Initialize Chip Select pin
 * @param hspi Pointer to SPI handle
 * @param csPort GPIO port for CS
 * @param csPin GPIO pin for CS
 * @param activeLevel Active level: 0=LOW, 1=HIGH
 */
void MIL_SPI_CS_Init(MIL_SPI_HandleTypeDef* hspi, MDR_PORT_TypeDef* csPort, uint16_t csPin, uint8_t activeLevel) {
  MIL_GPIO_InitTypeDef gpioInit;

  if (hspi == NULL || csPort == NULL) {
    return;
  }

  hspi->CSPins.CSPort        = csPort;
  hspi->CSPins.CSPin         = csPin;
  hspi->CSPins.CSActiveLevel = activeLevel;

  /* Configure CS pin as output */
  gpioInit.Pin        = csPin;
  gpioInit.Mode       = MIL_GPIO_MODE_OUTPUT;
  gpioInit.FuncMode   = MIL_GPIO_FUNC_PORT;
  gpioInit.PowerMode  = MIL_GPIO_POWER_MAX_FAST;
  gpioInit.Pull       = MIL_GPIO_NOPULL;
  gpioInit.DriverMode = MIL_GPIO_DRIVER_NORMAL;

  MIL_GPIO_Init(csPort, &gpioInit);

  /* Set initial state (inactive) */
  MIL_SPI_CS_Unselect(hspi);
}

/**
 * @brief Activate Chip Select
 * @param hspi Pointer to SPI handle
 */
void MIL_SPI_CS_Select(MIL_SPI_HandleTypeDef* hspi) {
  if (hspi->CSPins.CSPort == NULL) {
    return;
  }

  if (hspi->CSPins.CSActiveLevel == 0U) {
    MIL_GPIO_ResetPins(hspi->CSPins.CSPort, hspi->CSPins.CSPin);
  } else {
    MIL_GPIO_SetPins(hspi->CSPins.CSPort, hspi->CSPins.CSPin);
  }
}

/**
 * @brief Deactivate Chip Select
 * @param hspi Pointer to SPI handle
 */
void MIL_SPI_CS_Unselect(MIL_SPI_HandleTypeDef* hspi) {
  if (hspi->CSPins.CSPort == NULL) {
    return;
  }

  if (hspi->CSPins.CSActiveLevel == 0U) {
    MIL_GPIO_SetPins(hspi->CSPins.CSPort, hspi->CSPins.CSPin);
  } else {
    MIL_GPIO_ResetPins(hspi->CSPins.CSPort, hspi->CSPins.CSPin);
  }
}

/**
 * @brief Transmit data in polling mode
 * @param hspi Pointer to SPI handle
 * @param data Pointer to data buffer
 * @param size Number of data elements
 * @param timeout Timeout in milliseconds
 * @return MIL_SPI_State Operation status
 */
MIL_SPI_State MIL_SPI_Transmit(MIL_SPI_HandleTypeDef* hspi, void* data, uint16_t size, uint32_t timeout) {
  uint16_t i;
  uint8_t frameSize;
  uint16_t txValue;

  if (hspi == NULL || data == NULL || size == 0U) {
    return MIL_SPI_STATE_ERROR;
  }

  if (hspi->State != MIL_SPI_STATE_READY) {
    return MIL_SPI_STATE_BUSY_TX;
  }

  hspi->State     = MIL_SPI_STATE_BUSY_TX;
  hspi->ErrorCode = MIL_SPI_ERROR_NONE;
  frameSize       = MIL_SPI_GetFrameSize(hspi);

  for (i = 0U; i < size; i++) {
    /* Wait for TX FIFO not full */
    if (!_waitForTxNotFull(hspi, timeout)) {
      hspi->State     = MIL_SPI_STATE_READY;
      hspi->ErrorCode = MIL_SPI_ERROR_TIMEOUT;
      return MIL_SPI_STATE_ERROR;
    }

    /* Write data */
    if (frameSize == 1U) {
      txValue = ((uint8_t*)data)[i];
    } else {
      txValue = ((uint16_t*)data)[i];
    }
    MIL_SPI_WriteData(hspi, txValue);
  }

  /* Wait for transmission complete (TX empty and not busy) */
  if (!_waitForNotBusy(hspi, timeout)) {
    hspi->State     = MIL_SPI_STATE_READY;
    hspi->ErrorCode = MIL_SPI_ERROR_TIMEOUT;
    return MIL_SPI_STATE_ERROR;
  }

  /* Drain RX FIFO — in full-duplex SPI received data accumulates during TX */
  while (hspi->Instance->SR & SSP_SR_RNE) {
    (void)hspi->Instance->DR;
  }

  hspi->State = MIL_SPI_STATE_READY;
  return MIL_SPI_STATE_READY;
}

/**
 * @brief Receive data in polling mode
 * @param hspi Pointer to SPI handle
 * @param data Pointer to receive buffer
 * @param size Number of data elements
 * @param timeout Timeout in milliseconds
 * @return MIL_SPI_State Operation status
 */
MIL_SPI_State MIL_SPI_Receive(MIL_SPI_HandleTypeDef* hspi, void* data, uint16_t size, uint32_t timeout) {
  uint16_t i;
  uint8_t frameSize;
  uint16_t txDummy;
  uint16_t rxValue;

  if (hspi == NULL || data == NULL || size == 0U) {
    return MIL_SPI_STATE_ERROR;
  }

  if (hspi->State != MIL_SPI_STATE_READY) {
    return MIL_SPI_STATE_BUSY_RX;
  }

  hspi->State     = MIL_SPI_STATE_BUSY_RX;
  hspi->ErrorCode = MIL_SPI_ERROR_NONE;
  frameSize       = MIL_SPI_GetFrameSize(hspi);
  txDummy         = (frameSize == 1U) ? 0xFFU : 0xFFFFU;

  /* Flush any stale data from RX FIFO before starting receive */
  while (hspi->Instance->SR & SSP_SR_RNE) {
    (void)hspi->Instance->DR;
  }

  for (i = 0U; i < size; i++) {
    /* Wait for TX FIFO not full to send dummy byte */
    if (!_waitForTxNotFull(hspi, timeout)) {
      hspi->State     = MIL_SPI_STATE_READY;
      hspi->ErrorCode = MIL_SPI_ERROR_TIMEOUT;
      return MIL_SPI_STATE_ERROR;
    }

    /* Send dummy data to generate clock */
    MIL_SPI_WriteData(hspi, txDummy);

    /* Wait for RX data */
    if (!_waitForRxNotEmpty(hspi, timeout)) {
      hspi->State     = MIL_SPI_STATE_READY;
      hspi->ErrorCode = MIL_SPI_ERROR_TIMEOUT;
      return MIL_SPI_STATE_ERROR;
    }

    /* Read received data */
    rxValue = MIL_SPI_ReadData(hspi);
    if (frameSize == 1U) {
      ((uint8_t*)data)[i] = (uint8_t)rxValue;
    } else {
      ((uint16_t*)data)[i] = rxValue;
    }
  }

  hspi->State = MIL_SPI_STATE_READY;
  return MIL_SPI_STATE_READY;
}

/**
 * @brief Transmit and receive data in full-duplex mode
 * @param hspi Pointer to SPI handle
 * @param txData Pointer to transmit buffer (NULL for RX only)
 * @param rxData Pointer to receive buffer (NULL for TX only)
 * @param size Number of data elements
 * @param timeout Timeout in milliseconds
 * @return MIL_SPI_State Operation status
 */
MIL_SPI_State MIL_SPI_TransmitReceive(MIL_SPI_HandleTypeDef* hspi, void* txData, void* rxData, uint16_t size, uint32_t timeout) {
  uint16_t i;
  uint8_t frameSize;
  uint16_t txValue;
  uint16_t rxValue;

  if (hspi == NULL || size == 0U) {
    return MIL_SPI_STATE_ERROR;
  }

  if (txData == NULL && rxData == NULL) {
    return MIL_SPI_STATE_ERROR;
  }

  if (hspi->State != MIL_SPI_STATE_READY) {
    return MIL_SPI_STATE_BUSY_TX_RX;
  }

  hspi->State     = MIL_SPI_STATE_BUSY_TX_RX;
  hspi->ErrorCode = MIL_SPI_ERROR_NONE;
  frameSize       = MIL_SPI_GetFrameSize(hspi);

  /* Flush any stale data from RX FIFO before starting exchange */
  while (hspi->Instance->SR & SSP_SR_RNE) {
    (void)hspi->Instance->DR;
  }

  for (i = 0U; i < size; i++) {
    /* Wait for TX FIFO not full */
    if (!_waitForTxNotFull(hspi, timeout)) {
      hspi->State     = MIL_SPI_STATE_READY;
      hspi->ErrorCode = MIL_SPI_ERROR_TIMEOUT;
      return MIL_SPI_STATE_ERROR;
    }

    /* Write data (or dummy if txData is NULL) */
    if (txData != NULL) {
      if (frameSize == 1U) {
        txValue = ((uint8_t*)txData)[i];
      } else {
        txValue = ((uint16_t*)txData)[i];
      }
    } else {
      txValue = (frameSize == 1U) ? 0xFFU : 0xFFFFU;
    }
    MIL_SPI_WriteData(hspi, txValue);

    /* Wait for RX data */
    if (!_waitForRxNotEmpty(hspi, timeout)) {
      hspi->State     = MIL_SPI_STATE_READY;
      hspi->ErrorCode = MIL_SPI_ERROR_TIMEOUT;
      return MIL_SPI_STATE_ERROR;
    }

    /* Read data (discard if rxData is NULL) */
    rxValue = MIL_SPI_ReadData(hspi);
    if (rxData != NULL) {
      if (frameSize == 1U) {
        ((uint8_t*)rxData)[i] = (uint8_t)rxValue;
      } else {
        ((uint16_t*)rxData)[i] = rxValue;
      }
    }
  }

  /* Wait for completion */
  if (!_waitForNotBusy(hspi, timeout)) {
    hspi->State     = MIL_SPI_STATE_READY;
    hspi->ErrorCode = MIL_SPI_ERROR_TIMEOUT;
    return MIL_SPI_STATE_ERROR;
  }

  hspi->State = MIL_SPI_STATE_READY;
  return MIL_SPI_STATE_READY;
}

/**
 * @brief Calculate baud rate divisors
 * @param sysClock System clock frequency in Hz
 * @param targetBaud Target baud rate in Hz
 * @param prescaler Pointer to store CPSDVSR value
 * @param divider Pointer to store SCR value
 * @return Actual baud rate in Hz
 */
uint32_t MIL_SPI_CalculateBaudRate(uint32_t sysClock, uint32_t targetBaud, uint8_t* prescaler, uint8_t* divider) {
  uint32_t bestBaud = 0U;
  uint32_t actualBaud;
  uint32_t error;
  uint32_t minError   = 0xFFFFFFFFU;
  uint8_t bestCPSDVSR = 2U;
  uint8_t bestSCR     = 0U;
  uint8_t cpsdvsr, scr;

  if (prescaler == NULL || divider == NULL || targetBaud == 0U) {
    return 0U;
  }

  /* Try all even CPSDVSR values from 2 to 254 */
  for (cpsdvsr = 2U; cpsdvsr <= 254U; cpsdvsr += 2U) {
    for (scr = 0U; scr <= 255U; scr++) {
      actualBaud = sysClock / ((uint32_t)cpsdvsr * ((uint32_t)scr + 1U));

      if (actualBaud <= targetBaud) {
        error = targetBaud - actualBaud;
        if (error < minError) {
          minError    = error;
          bestCPSDVSR = cpsdvsr;
          bestSCR     = scr;
          bestBaud    = actualBaud;

          /* Perfect match */
          if (error == 0U) {
            *prescaler = bestCPSDVSR;
            *divider   = bestSCR;
            return bestBaud;
          }
        }
      }
    }
  }

  *prescaler = bestCPSDVSR;
  *divider   = bestSCR;
  return bestBaud;
}

/**
 * @brief Clear interrupt flags
 * @param hspi Pointer to SPI handle
 * @param flags Flags to clear (SSP_ICR_RORIC, SSP_ICR_RTIC)
 */
void MIL_SPI_ClearInterruptFlags(MIL_SPI_HandleTypeDef* hspi, uint32_t flags) {
  if (hspi != NULL) {
    hspi->Instance->ICR = flags;
  }
}

/**
 * @brief Enable interrupts
 * @param hspi Pointer to SPI handle
 * @param txEnable Enable TX interrupt
 * @param rxEnable Enable RX interrupt
 * @param overrunEnable Enable overrun interrupt
 * @param timeoutEnable Enable timeout interrupt
 */
void MIL_SPI_EnableInterrupts(MIL_SPI_HandleTypeDef* hspi, uint8_t txEnable, uint8_t rxEnable, uint8_t overrunEnable, uint8_t timeoutEnable) {
  uint32_t imsc = 0U;

  if (hspi == NULL) {
    return;
  }

  if (txEnable) {
    imsc |= SSP_IMSC_TXIM;
  }
  if (rxEnable) {
    imsc |= SSP_IMSC_RXIM;
  }
  if (overrunEnable) {
    imsc |= SSP_IMSC_RORIM;
  }
  if (timeoutEnable) {
    imsc |= SSP_IMSC_RTIM;
  }

  hspi->Instance->IMSC = imsc;

  /* Enable NVIC interrupt */
  if (hspi->Instance == MDR_SSP1) {
    NVIC_EnableIRQ(SSP1_IRQn);
  } else if (hspi->Instance == MDR_SSP2) {
    NVIC_EnableIRQ(SSP2_IRQn);
  }
}

/**
 * @brief Disable all interrupts
 * @param hspi Pointer to SPI handle
 */
void MIL_SPI_DisableInterrupts(MIL_SPI_HandleTypeDef* hspi) {
  if (hspi == NULL) {
    return;
  }

  hspi->Instance->IMSC = 0U;

  /* Disable NVIC interrupt */
  if (hspi->Instance == MDR_SSP1) {
    NVIC_DisableIRQ(SSP1_IRQn);
  } else if (hspi->Instance == MDR_SSP2) {
    NVIC_DisableIRQ(SSP2_IRQn);
  }
}

/**
 * @brief Transmit data in interrupt mode
 * @param hspi Pointer to SPI handle
 * @param data Pointer to data buffer
 * @param size Number of data elements
 * @return MIL_SPI_State Operation status
 */
MIL_SPI_State MIL_SPI_Transmit_IT(MIL_SPI_HandleTypeDef* hspi, void* data, uint16_t size) {
  uint16_t txValue;

  if (hspi == NULL || data == NULL || size == 0U) {
    return MIL_SPI_STATE_ERROR;
  }

  if (hspi->State != MIL_SPI_STATE_READY) {
    return MIL_SPI_STATE_BUSY_TX;
  }

  hspi->State     = MIL_SPI_STATE_BUSY_TX;
  hspi->ErrorCode = MIL_SPI_ERROR_NONE;
  hspi->pTxBuffer = data;
  hspi->TxSize    = size;
  hspi->TxCount   = 0U;
  hspi->pRxBuffer = NULL;
  hspi->RxSize    = 0U;

  /* Fill TX FIFO initially (up to 8 words) */
  while ((hspi->TxCount < hspi->TxSize) && (hspi->Instance->SR & SSP_SR_TNF)) {
    if (MIL_SPI_GetFrameSize(hspi) == 1U) {
      txValue = ((uint8_t*)hspi->pTxBuffer)[hspi->TxCount];
    } else {
      txValue = ((uint16_t*)hspi->pTxBuffer)[hspi->TxCount];
    }
    MIL_SPI_WriteData(hspi, txValue);
    hspi->TxCount++;
  }

  /* Enable TX interrupt */
  MIL_SPI_EnableInterrupts(hspi, 1U, 0U, 1U, 0U);

  return MIL_SPI_STATE_BUSY_TX;
}

/**
 * @brief Receive data in interrupt mode
 * @param hspi Pointer to SPI handle
 * @param data Pointer to receive buffer
 * @param size Number of data elements
 * @return MIL_SPI_State Operation status
 */
MIL_SPI_State MIL_SPI_Receive_IT(MIL_SPI_HandleTypeDef* hspi, void* data, uint16_t size) {
  uint16_t txDummy;

  if (hspi == NULL || data == NULL || size == 0U) {
    return MIL_SPI_STATE_ERROR;
  }

  if (hspi->State != MIL_SPI_STATE_READY) {
    return MIL_SPI_STATE_BUSY_RX;
  }

  hspi->State     = MIL_SPI_STATE_BUSY_RX;
  hspi->ErrorCode = MIL_SPI_ERROR_NONE;
  hspi->pRxBuffer = data;
  hspi->RxSize    = size;
  hspi->RxCount   = 0U;
  hspi->pTxBuffer = NULL;
  hspi->TxSize    = size; /* Need to transmit dummy bytes */
  hspi->TxCount   = 0U;
  txDummy         = (MIL_SPI_GetFrameSize(hspi) == 1U) ? 0xFFU : 0xFFFFU;

  /* Send initial dummy bytes to start clock (up to 8 words) */
  while ((hspi->TxCount < hspi->TxSize) && (hspi->Instance->SR & SSP_SR_TNF)) {
    MIL_SPI_WriteData(hspi, txDummy);
    hspi->TxCount++;
  }

  /* Enable RX and TX interrupts */
  MIL_SPI_EnableInterrupts(hspi, 1U, 1U, 1U, 0U);

  return MIL_SPI_STATE_BUSY_RX;
}

/**
 * @brief Transmit and receive in interrupt mode
 * @param hspi Pointer to SPI handle
 * @param txData Pointer to transmit buffer
 * @param rxData Pointer to receive buffer
 * @param size Number of data elements
 * @return MIL_SPI_State Operation status
 */
MIL_SPI_State MIL_SPI_TransmitReceive_IT(MIL_SPI_HandleTypeDef* hspi, void* txData, void* rxData, uint16_t size) {
  uint16_t txValue;

  if (hspi == NULL || size == 0U) {
    return MIL_SPI_STATE_ERROR;
  }

  if (hspi->State != MIL_SPI_STATE_READY) {
    return MIL_SPI_STATE_BUSY_TX_RX;
  }

  hspi->State     = MIL_SPI_STATE_BUSY_TX_RX;
  hspi->ErrorCode = MIL_SPI_ERROR_NONE;
  hspi->pTxBuffer = txData;
  hspi->TxSize    = size;
  hspi->TxCount   = 0U;
  hspi->pRxBuffer = rxData;
  hspi->RxSize    = size;
  hspi->RxCount   = 0U;

  /* Fill TX FIFO initially */
  while ((hspi->TxCount < hspi->TxSize) && (hspi->Instance->SR & SSP_SR_TNF)) {
    if (txData != NULL) {
      if (MIL_SPI_GetFrameSize(hspi) == 1U) {
        txValue = ((uint8_t*)txData)[hspi->TxCount];
      } else {
        txValue = ((uint16_t*)txData)[hspi->TxCount];
      }
    } else {
      txValue = (MIL_SPI_GetFrameSize(hspi) == 1U) ? 0xFFU : 0xFFFFU;
    }
    MIL_SPI_WriteData(hspi, txValue);
    hspi->TxCount++;
  }

  /* Enable TX and RX interrupts */
  MIL_SPI_EnableInterrupts(hspi, 1U, 1U, 1U, 0U);

  return MIL_SPI_STATE_BUSY_TX_RX;
}

/**
 * @brief SPI interrupt handler
 * @param hspi Pointer to SPI handle
 */
void MIL_SPI_IRQHandler(MIL_SPI_HandleTypeDef* hspi) {
  uint32_t mis;
  uint8_t frameSize;
  uint16_t txValue;
  uint16_t rxValue;

  if (hspi == NULL) {
    return;
  }

  mis       = hspi->Instance->MIS;
  frameSize = MIL_SPI_GetFrameSize(hspi);

  /* TX interrupt - TX FIFO half empty or less */
  if (mis & SSP_MIS_TXMIS) {
    if (hspi->State == MIL_SPI_STATE_BUSY_TX) {
      /* Continue transmission */
      while ((hspi->TxCount < hspi->TxSize) && (hspi->Instance->SR & SSP_SR_TNF)) {
        if (frameSize == 1U) {
          txValue = ((uint8_t*)hspi->pTxBuffer)[hspi->TxCount];
        } else {
          txValue = ((uint16_t*)hspi->pTxBuffer)[hspi->TxCount];
        }
        MIL_SPI_WriteData(hspi, txValue);
        hspi->TxCount++;
      }

      /* Check if complete */
      if (hspi->TxCount >= hspi->TxSize) {
        /* Wait for not busy */
        if (!(hspi->Instance->SR & SSP_SR_BSY)) {
          MIL_SPI_DisableInterrupts(hspi);
          hspi->State = MIL_SPI_STATE_READY;
          if (hspi->TxCpltCallback != NULL) {
            hspi->TxCpltCallback(hspi);
          }
        }
      }
    } else if (hspi->State == MIL_SPI_STATE_BUSY_RX || hspi->State == MIL_SPI_STATE_BUSY_TX_RX) {
      /* For RX mode, continue sending dummy bytes */
      while ((hspi->TxCount < hspi->TxSize) && (hspi->Instance->SR & SSP_SR_TNF)) {
        MIL_SPI_WriteData(hspi, (frameSize == 1U) ? 0xFFU : 0xFFFFU);
        hspi->TxCount++;
      }
    }
  }

  /* RX interrupt - RX FIFO half full or more */
  if (mis & SSP_MIS_RXMIS) {
    /* Read received data */
    while ((hspi->Instance->SR & SSP_SR_RNE) && (hspi->RxCount < hspi->RxSize)) {
      rxValue = MIL_SPI_ReadData(hspi);
      if (hspi->pRxBuffer != NULL) {
        if (frameSize == 1U) {
          ((uint8_t*)hspi->pRxBuffer)[hspi->RxCount] = (uint8_t)rxValue;
        } else {
          ((uint16_t*)hspi->pRxBuffer)[hspi->RxCount] = rxValue;
        }
      }
      hspi->RxCount++;
    }

    /* Check if receive complete */
    if (hspi->RxCount >= hspi->RxSize) {
      if (hspi->State == MIL_SPI_STATE_BUSY_RX) {
        MIL_SPI_DisableInterrupts(hspi);
        hspi->State = MIL_SPI_STATE_READY;
        if (hspi->RxCpltCallback != NULL) {
          hspi->RxCpltCallback(hspi);
        }
      } else if (hspi->State == MIL_SPI_STATE_BUSY_TX_RX) {
        if (hspi->TxCount >= hspi->TxSize) {
          /* Both TX and RX complete */
          MIL_SPI_DisableInterrupts(hspi);
          hspi->State = MIL_SPI_STATE_READY;
          if (hspi->TxRxCpltCallback != NULL) {
            hspi->TxRxCpltCallback(hspi);
          }
        }
      }
    }
  }

  /* Overrun interrupt */
  if (mis & SSP_MIS_RORMIS) {
    hspi->ErrorCode |= MIL_SPI_ERROR_OVERRUN;
    MIL_SPI_ClearInterruptFlags(hspi, SSP_ICR_RORIC);
    hspi->State = MIL_SPI_STATE_ERROR;
    if (hspi->ErrorCallback != NULL) {
      hspi->ErrorCallback(hspi);
    }
  }

  /* Timeout interrupt */
  if (mis & SSP_MIS_RTMIS) {
    MIL_SPI_ClearInterruptFlags(hspi, SSP_ICR_RTIC);
  }
}

/**
 * @brief Register callback functions
 * @param hspi Pointer to SPI handle
 * @param txCplt TX complete callback
 * @param rxCplt RX complete callback
 * @param txRxCplt TX/RX complete callback
 * @param error Error callback
 */
void MIL_SPI_RegisterCallbacks(MIL_SPI_HandleTypeDef* hspi,
    void (*txCplt)(MIL_SPI_HandleTypeDef*),
    void (*rxCplt)(MIL_SPI_HandleTypeDef*),
    void (*txRxCplt)(MIL_SPI_HandleTypeDef*),
    void (*error)(MIL_SPI_HandleTypeDef*)) {
  if (hspi == NULL) {
    return;
  }

  hspi->TxCpltCallback   = txCplt;
  hspi->RxCpltCallback   = rxCplt;
  hspi->TxRxCpltCallback = txRxCplt;
  hspi->ErrorCallback    = error;
}
