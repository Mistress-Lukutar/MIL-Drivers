/**
 * @file MIL_SPI.h
 * @brief SPI/SSP driver for 1986BE9x series MCUs.
 *
 * @author Mistress-Lukutar
 * @date   2026-05-18
 * @version 1.0.0
 */

#ifndef MIL_SPI_H
#define MIL_SPI_H

#include "MDR32Fx.h"
#include "MIL_GPIO.h"
#include <stddef.h>
#include <stdint.h>

/**
 * @brief SPI instance selection
 */
typedef enum {
  MIL_SPI_1 = 0, /**< SSP1 module */
  MIL_SPI_2 = 1  /**< SSP2 module */
} MIL_SPI_Instance;

/**
 * @brief SPI operating mode
 */
typedef enum {
  MIL_SPI_MODE_MASTER = 0, /**< Master mode */
  MIL_SPI_MODE_SLAVE  = 1  /**< Slave mode */
} MIL_SPI_Mode;

/**
 * @brief SPI protocol selection
 */
typedef enum {
  MIL_SPI_PROTOCOL_SPI       = SSP_CR0_FRF_SPI_MOT, /**< Motorola SPI */
  MIL_SPI_PROTOCOL_SSI       = SSP_CR0_FRF_SSI_TI,  /**< Texas Instruments SSI */
  MIL_SPI_PROTOCOL_MICROWIRE = SSP_CR0_FRF_MW_NS    /**< National Microwire */
} MIL_SPI_Protocol;

/**
 * @brief SPI mode (CPOL/CPHA combination)
 */
typedef enum {
  MIL_SPI_MODE_0 = 0,                        /**< CPOL=0, CPHA=0 */
  MIL_SPI_MODE_1 = SSP_CR0_SPH,              /**< CPOL=0, CPHA=1 */
  MIL_SPI_MODE_2 = SSP_CR0_SPO,              /**< CPOL=1, CPHA=0 */
  MIL_SPI_MODE_3 = SSP_CR0_SPO | SSP_CR0_SPH /**< CPOL=1, CPHA=1 */
} MIL_SPI_ModeType;

/**
 * @brief SPI data size
 */
typedef enum {
  MIL_SPI_DATASIZE_4BIT  = SSP_CR0_DSS_4_BITS,
  MIL_SPI_DATASIZE_5BIT  = SSP_CR0_DSS_5_BITS,
  MIL_SPI_DATASIZE_6BIT  = SSP_CR0_DSS_6_BITS,
  MIL_SPI_DATASIZE_7BIT  = SSP_CR0_DSS_7_BITS,
  MIL_SPI_DATASIZE_8BIT  = SSP_CR0_DSS_8_BITS,
  MIL_SPI_DATASIZE_9BIT  = SSP_CR0_DSS_9_BITS,
  MIL_SPI_DATASIZE_10BIT = SSP_CR0_DSS_10_BITS,
  MIL_SPI_DATASIZE_11BIT = SSP_CR0_DSS_11_BITS,
  MIL_SPI_DATASIZE_12BIT = SSP_CR0_DSS_12_BITS,
  MIL_SPI_DATASIZE_13BIT = SSP_CR0_DSS_13_BITS,
  MIL_SPI_DATASIZE_14BIT = SSP_CR0_DSS_14_BITS,
  MIL_SPI_DATASIZE_15BIT = SSP_CR0_DSS_15_BITS,
  MIL_SPI_DATASIZE_16BIT = SSP_CR0_DSS_16_BITS
} MIL_SPI_DataSize;

/**
 * @brief SPI driver state
 */
typedef enum {
  MIL_SPI_STATE_RESET = 0,  /**< Not initialized */
  MIL_SPI_STATE_READY,      /**< Ready for operation */
  MIL_SPI_STATE_BUSY_TX,    /**< Transmit in progress */
  MIL_SPI_STATE_BUSY_RX,    /**< Receive in progress */
  MIL_SPI_STATE_BUSY_TX_RX, /**< Full duplex in progress */
  MIL_SPI_STATE_ERROR       /**< Error occurred */
} MIL_SPI_State;

/**
 * @brief SPI error codes
 */
typedef enum {
  MIL_SPI_ERROR_NONE    = 0x00, /**< No error */
  MIL_SPI_ERROR_TIMEOUT = 0x01, /**< Timeout error */
  MIL_SPI_ERROR_OVERRUN = 0x02, /**< RX FIFO overrun */
  MIL_SPI_ERROR_DMA     = 0x04, /**< DMA error */
  MIL_SPI_ERROR_FLAG    = 0x08  /**< Status flag error */
} MIL_SPI_ErrorCode;

/**
 * @brief Chip Select configuration
 */
typedef struct {
  MDR_PORT_TypeDef* CSPort; /**< GPIO port for CS (NULL if not used) */
  uint16_t CSPin;           /**< GPIO pin for CS */
  uint8_t CSActiveLevel;    /**< Active level: 0=LOW, 1=HIGH */
} MIL_SPI_CSPinsTypeDef;

/* Forward declaration for callback typedef */
typedef struct MIL_SPI_HandleTypeDef MIL_SPI_HandleTypeDef;

/**
 * @brief SPI handle structure
 */
struct MIL_SPI_HandleTypeDef {
  MDR_SSP_TypeDef* Instance;    /**< SSP registers */
  MIL_SPI_Mode Mode;            /**< Master/Slave */
  MIL_SPI_Protocol Protocol;    /**< SPI/SSI/Microwire */
  MIL_SPI_DataSize DataSize;    /**< Data frame size */
  MIL_SPI_ModeType ModeType;    /**< SPI mode 0-3 */
  uint8_t Prescaler;            /**< CPSDVSR value (2-254, even) */
  uint8_t Divider;              /**< SCR value (0-255) */
  MIL_SPI_CSPinsTypeDef CSPins; /**< Chip Select pins */
  MIL_SPI_State State;          /**< Current state */
  uint32_t ErrorCode;           /**< Error code bitmask */

  /* Interrupt mode fields */
  void* pTxBuffer;                                       /**< TX buffer pointer */
  void* pRxBuffer;                                       /**< RX buffer pointer */
  uint16_t TxSize;                                       /**< TX size remaining */
  uint16_t RxSize;                                       /**< RX size remaining */
  uint16_t TxCount;                                      /**< TX counter */
  uint16_t RxCount;                                      /**< RX counter */
  void (*TxCpltCallback)(MIL_SPI_HandleTypeDef* hspi);   /**< TX complete callback */
  void (*RxCpltCallback)(MIL_SPI_HandleTypeDef* hspi);   /**< RX complete callback */
  void (*TxRxCpltCallback)(MIL_SPI_HandleTypeDef* hspi); /**< TX/RX complete callback */
  void (*ErrorCallback)(MIL_SPI_HandleTypeDef* hspi);    /**< Error callback */
};

/**
 * @brief Get frame size in bytes from DataSize configuration
 * @param hspi Pointer to SPI handle
 * @return 1 for 4-8 bit frames, 2 for 9-16 bit frames
 */
static inline uint8_t MIL_SPI_GetFrameSize(const MIL_SPI_HandleTypeDef* hspi) { return (hspi->DataSize <= MIL_SPI_DATASIZE_8BIT) ? 1U : 2U; }

/**
 * @brief Initialize SPI peripheral
 * @param hspi Pointer to SPI handle
 * @return MIL_SPI_State Current state
 */
MIL_SPI_State MIL_SPI_Init(MIL_SPI_HandleTypeDef* hspi);

/**
 * @brief Deinitialize SPI peripheral
 * @param hspi Pointer to SPI handle
 * @return MIL_SPI_State Current state
 */
MIL_SPI_State MIL_SPI_DeInit(MIL_SPI_HandleTypeDef* hspi);

/**
 * @brief Initialize GPIO for SPI (CLK, RXD, TXD)
 * @param hspi Pointer to SPI handle
 * @param port GPIO port for SPI pins
 * @note Uses alternate function mode for SPI pins
 */
void MIL_SPI_GPIO_Init(MIL_SPI_HandleTypeDef* hspi, MDR_PORT_TypeDef* port);

/**
 * @brief Initialize Chip Select pin
 * @param hspi Pointer to SPI handle
 * @param csPort GPIO port for CS
 * @param csPin GPIO pin for CS
 * @param activeLevel Active level: 0=LOW, 1=HIGH
 */
void MIL_SPI_CS_Init(MIL_SPI_HandleTypeDef* hspi, MDR_PORT_TypeDef* csPort, uint16_t csPin, uint8_t activeLevel);

/**
 * @brief Transmit data in polling mode
 * @param hspi Pointer to SPI handle
 * @param data Pointer to data buffer
 * @param size Number of data elements
 * @param timeout Timeout in milliseconds
 * @return MIL_SPI_State Operation status
 */
MIL_SPI_State MIL_SPI_Transmit(MIL_SPI_HandleTypeDef* hspi, void* data, uint16_t size, uint32_t timeout);

/**
 * @brief Receive data in polling mode
 * @param hspi Pointer to SPI handle
 * @param data Pointer to receive buffer
 * @param size Number of data elements
 * @param timeout Timeout in milliseconds
 * @return MIL_SPI_State Operation status
 * @note Dummy data is transmitted to generate clock
 */
MIL_SPI_State MIL_SPI_Receive(MIL_SPI_HandleTypeDef* hspi, void* data, uint16_t size, uint32_t timeout);

/**
 * @brief Transmit and receive data in full-duplex mode
 * @param hspi Pointer to SPI handle
 * @param txData Pointer to transmit buffer (NULL for RX only)
 * @param rxData Pointer to receive buffer (NULL for TX only)
 * @param size Number of data elements
 * @param timeout Timeout in milliseconds
 * @return MIL_SPI_State Operation status
 */
MIL_SPI_State MIL_SPI_TransmitReceive(MIL_SPI_HandleTypeDef* hspi, void* txData, void* rxData, uint16_t size, uint32_t timeout);

/**
 * @brief Activate Chip Select
 * @param hspi Pointer to SPI handle
 */
void MIL_SPI_CS_Select(MIL_SPI_HandleTypeDef* hspi);

/**
 * @brief Deactivate Chip Select
 * @param hspi Pointer to SPI handle
 */
void MIL_SPI_CS_Unselect(MIL_SPI_HandleTypeDef* hspi);

/**
 * @brief Enable SPI peripheral
 * @param hspi Pointer to SPI handle
 */
static inline void MIL_SPI_Enable(MIL_SPI_HandleTypeDef* hspi) { hspi->Instance->CR1 |= SSP_CR1_SSE; }

/**
 * @brief Disable SPI peripheral
 * @param hspi Pointer to SPI handle
 */
static inline void MIL_SPI_Disable(MIL_SPI_HandleTypeDef* hspi) { hspi->Instance->CR1 &= ~SSP_CR1_SSE; }

/**
 * @brief Check if SPI is busy
 * @param hspi Pointer to SPI handle
 * @return 1 if busy, 0 if idle
 */
static inline uint8_t MIL_SPI_IsBusy(MIL_SPI_HandleTypeDef* hspi) { return (hspi->Instance->SR & SSP_SR_BSY) ? 1U : 0U; }

/**
 * @brief Check if RX FIFO is not empty
 * @param hspi Pointer to SPI handle
 * @return 1 if data available, 0 if empty
 */
static inline uint8_t MIL_SPI_IsRxNotEmpty(MIL_SPI_HandleTypeDef* hspi) { return (hspi->Instance->SR & SSP_SR_RNE) ? 1U : 0U; }

/**
 * @brief Check if TX FIFO is not full
 * @param hspi Pointer to SPI handle
 * @return 1 if can write, 0 if full
 */
static inline uint8_t MIL_SPI_IsTxNotFull(MIL_SPI_HandleTypeDef* hspi) { return (hspi->Instance->SR & SSP_SR_TNF) ? 1U : 0U; }

/**
 * @brief Read data from DR register
 * @param hspi Pointer to SPI handle
 * @return Received data
 */
static inline uint16_t MIL_SPI_ReadData(MIL_SPI_HandleTypeDef* hspi) { return (uint16_t)hspi->Instance->DR; }

/**
 * @brief Write data to DR register
 * @param hspi Pointer to SPI handle
 * @param data Data to write
 */
static inline void MIL_SPI_WriteData(MIL_SPI_HandleTypeDef* hspi, uint16_t data) { hspi->Instance->DR = data; }

/**
 * @brief Calculate baud rate divisors
 * @param sysClock System clock frequency in Hz
 * @param targetBaud Target baud rate in Hz
 * @param prescaler Pointer to store CPSDVSR value
 * @param divider Pointer to store SCR value
 * @return Actual baud rate in Hz
 * @note Formula: F = Fsspclk / (CPSDVSR * (1 + SCR))
 */
uint32_t MIL_SPI_CalculateBaudRate(uint32_t sysClock, uint32_t targetBaud, uint8_t* prescaler, uint8_t* divider);

/**
 * @brief Clear interrupt flags
 * @param hspi Pointer to SPI handle
 * @param flags Flags to clear (SSP_ICR_RORIC, SSP_ICR_RTIC)
 */
void MIL_SPI_ClearInterruptFlags(MIL_SPI_HandleTypeDef* hspi, uint32_t flags);

/**
 * @brief Enable interrupts
 * @param hspi Pointer to SPI handle
 * @param txEnable Enable TX interrupt
 * @param rxEnable Enable RX interrupt
 * @param overrunEnable Enable overrun interrupt
 * @param timeoutEnable Enable timeout interrupt
 */
void MIL_SPI_EnableInterrupts(MIL_SPI_HandleTypeDef* hspi, uint8_t txEnable, uint8_t rxEnable, uint8_t overrunEnable, uint8_t timeoutEnable);

/**
 * @brief Disable all interrupts
 * @param hspi Pointer to SPI handle
 */
void MIL_SPI_DisableInterrupts(MIL_SPI_HandleTypeDef* hspi);

/**
 * @brief Transmit data in interrupt mode
 * @param hspi Pointer to SPI handle
 * @param data Pointer to data buffer
 * @param size Number of data elements
 * @return MIL_SPI_State Operation status
 */
MIL_SPI_State MIL_SPI_Transmit_IT(MIL_SPI_HandleTypeDef* hspi, void* data, uint16_t size);

/**
 * @brief Receive data in interrupt mode
 * @param hspi Pointer to SPI handle
 * @param data Pointer to receive buffer
 * @param size Number of data elements
 * @return MIL_SPI_State Operation status
 */
MIL_SPI_State MIL_SPI_Receive_IT(MIL_SPI_HandleTypeDef* hspi, void* data, uint16_t size);

/**
 * @brief Transmit and receive in interrupt mode
 * @param hspi Pointer to SPI handle
 * @param txData Pointer to transmit buffer
 * @param rxData Pointer to receive buffer
 * @param size Number of data elements
 * @return MIL_SPI_State Operation status
 */
MIL_SPI_State MIL_SPI_TransmitReceive_IT(MIL_SPI_HandleTypeDef* hspi, void* txData, void* rxData, uint16_t size);

/**
 * @brief SPI interrupt handler
 * @param hspi Pointer to SPI handle
 * @note Call from SSP1_IRQHandler or SSP2_IRQHandler
 */
void MIL_SPI_IRQHandler(MIL_SPI_HandleTypeDef* hspi);

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
    void (*error)(MIL_SPI_HandleTypeDef*));

#endif /* MIL_SPI_H */
