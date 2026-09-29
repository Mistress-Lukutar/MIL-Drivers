/**
 * @file MIL_Uart.h
 * @brief Universal UART driver for MDR1986VE9x series MCUs (half-/full-duplex)
 *
 * Supports both UART1 and UART2 with configurable GPIO pins.
 * Provides blocking and interrupt-based transmission/reception.
 *
 * @author Mistress-Lukutar
 * @date   2026-09-29
 * @version v2.1.1
 */

#ifndef MIL_UART_H
#define MIL_UART_H

#include "MDR32Fx.h"
#include <stdint.h>

/* ======================== Buffer Configuration ======================== */

/** @defgroup UART_Buffer_Sizes Buffer Size Definitions
 * @{
 */
#define UART_RX_BUFFER_SIZE 64   /**< Receive buffer size in bytes */
#define UART_TX_BUFFER_SIZE 1024 /**< Transmit buffer size in bytes */
/** @} */

/* ======================== UART Operating Mode ======================== */

/** @defgroup UART_Mode UART Operating Mode
 * @{
 */
typedef enum {
  MIL_UART_MODE_HALF_DUPLEX = 0, /**< Half-duplex: TX and RX share direction control */
  MIL_UART_MODE_FULL_DUPLEX = 1  /**< Full-duplex: simultaneous TX and RX */
} MIL_UART_ModeTypeDef;
/** @} */

/* ======================== Half-Duplex Direction ======================== */

/** @defgroup UART_Direction Half-Duplex Direction
 * @{
 */
#define MIL_UART_DIRECTION_RX 0 /**< Receive mode */
#define MIL_UART_DIRECTION_TX 1 /**< Transmit mode */
/** @} */

/* ======================== UART Status ======================== */

/**
 * @brief UART operational status enumeration
 */
typedef enum {
  MIL_UART_READY = 0, /**< UART is ready for operation */
  MIL_UART_BUSY,      /**< UART is busy transmitting */
  MIL_UART_ERROR      /**< UART encountered an error */
} MIL_UART_StatusTypeDef;

/**
 * @brief UART operation result codes
 */
typedef enum {
  MIL_UART_OK = 0,             /**< Operation completed successfully */
  MIL_UART_ERROR_TIMEOUT,      /**< Operation timed out */
  MIL_UART_ERROR_BUFFER_FULL,  /**< Transmit buffer is full */
  MIL_UART_ERROR_INVALID_PARAM /**< Invalid parameter provided */
} MIL_UART_ErrorTypeDef;

/* ======================== GPIO Pin Configuration ======================== */

/**
 * @brief GPIO pin configuration structure
 */
typedef struct {
  MDR_PORT_TypeDef* port; /**< GPIO port (e.g., MDR_PORTA) */
  uint8_t pin;            /**< Pin number (0-15) */
} MIL_UART_PinTypeDef;

/* ======================== UART Handle Structure ======================== */

/**
 * @brief UART handle structure with full configuration
 */
typedef struct {
  MDR_UART_TypeDef* instance; /**< UART peripheral instance */
  MIL_UART_PinTypeDef txPin;  /**< TX pin configuration */
  MIL_UART_PinTypeDef rxPin;  /**< RX pin configuration */

  uint8_t clkDiv; /**< Clock divider value */
  uint16_t ibrd;  /**< Integer baud rate divisor */
  uint8_t fbrd;   /**< Fractional baud rate divisor */

  MIL_UART_ModeTypeDef mode; /**< Operating mode: half-duplex or full-duplex */

  uint8_t rxBuffer[UART_RX_BUFFER_SIZE]; /**< Receive circular buffer */
  uint8_t txBuffer[UART_TX_BUFFER_SIZE]; /**< Transmit circular buffer */

  volatile uint16_t txHead; /**< Transmit buffer head index */
  volatile uint16_t txTail; /**< Transmit buffer tail index */
  volatile uint16_t rxHead; /**< Receive buffer head index */
  volatile uint16_t rxTail; /**< Receive buffer tail index */

  volatile MIL_UART_StatusTypeDef status; /**< Current UART status */
} MIL_UART_HandleTypeDef;

/* ======================== Function Prototypes ======================== */

/**
 * @brief Initialize UART peripheral in blocking mode
 *
 * Configures GPIO pins, enables clocks, sets baud rate and frame format.
 * FIFO is enabled for efficient data handling.
 *
 * @param uart Pointer to UART handle structure
 * @return MIL_UART_OK on success, error code otherwise
 */
MIL_UART_ErrorTypeDef MIL_UART_Init(MIL_UART_HandleTypeDef* uart);

/**
 * @brief Initialize UART peripheral in interrupt mode
 *
 * Similar to MIL_UART_Init but enables UART interrupts and
 * configures NVIC for asynchronous operation.
 *
 * @param uart Pointer to UART handle structure
 * @return MIL_UART_OK on success, error code otherwise
 */
MIL_UART_ErrorTypeDef MIL_UART_InitIT(MIL_UART_HandleTypeDef* uart);

/**
 * @brief Send data in blocking mode with timeout
 *
 * Transmits data byte-by-byte with timeout protection.
 * Automatically switches to TX mode and back to RX after transmission.
 *
 * @param uart Pointer to UART handle structure
 * @param data Pointer to data buffer to transmit
 * @param length Number of bytes to transmit
 * @param timeout Timeout in milliseconds
 * @return MIL_UART_OK on success, MIL_UART_ERROR_TIMEOUT on timeout
 */
MIL_UART_ErrorTypeDef MIL_UART_Send(MIL_UART_HandleTypeDef* uart, const uint8_t* data, uint16_t length, uint32_t timeout);

/**
 * @brief Send null-terminated string in blocking mode
 *
 * @param uart Pointer to UART handle structure
 * @param str Pointer to null-terminated string
 * @param timeout Timeout in milliseconds
 * @return MIL_UART_OK on success, error code otherwise
 */
MIL_UART_ErrorTypeDef MIL_UART_SendString(MIL_UART_HandleTypeDef* uart, const char* str, uint32_t timeout);

/**
 * @brief Send data in interrupt mode (non-blocking)
 *
 * Queues data in transmit buffer and initiates interrupt-driven transmission.
 * Returns immediately without waiting for completion.
 *
 * @param uart Pointer to UART handle structure
 * @param data Pointer to data buffer to transmit
 * @param length Number of bytes to transmit
 * @return MIL_UART_OK on success, MIL_UART_ERROR_BUFFER_FULL if buffer full
 */
MIL_UART_ErrorTypeDef MIL_UART_SendIT(MIL_UART_HandleTypeDef* uart, const uint8_t* data, uint16_t length);

/**
 * @brief Send string in interrupt mode (non-blocking)
 *
 * @param uart Pointer to UART handle structure
 * @param str Pointer to null-terminated string
 * @return MIL_UART_OK on success, error code otherwise
 */
MIL_UART_ErrorTypeDef MIL_UART_SendStringIT(MIL_UART_HandleTypeDef* uart, const char* str);

/**
 * @brief Receive data in blocking mode with timeout
 *
 * Receives specified number of bytes with timeout protection.
 *
 * @param uart Pointer to UART handle structure
 * @param data Pointer to buffer for received data
 * @param length Number of bytes to receive
 * @param timeout Timeout in milliseconds
 * @return MIL_UART_OK on success, MIL_UART_ERROR_TIMEOUT on timeout
 */
MIL_UART_ErrorTypeDef MIL_UART_Receive(MIL_UART_HandleTypeDef* uart, uint8_t* data, uint16_t length, uint32_t timeout);

/**
 * @brief Get number of bytes available in receive buffer
 *
 * @param uart Pointer to UART handle structure
 * @return Number of bytes available to read
 */
uint16_t MIL_UART_GetRxAvailable(const MIL_UART_HandleTypeDef* uart);

/**
 * @brief Get number of free bytes in transmit buffer
 *
 * @param uart Pointer to UART handle structure
 * @return Number of free bytes available to write
 */
uint16_t MIL_UART_GetTxFree(const MIL_UART_HandleTypeDef* uart);

/**
 * @brief Read one byte from receive buffer (interrupt mode)
 *
 * @param uart Pointer to UART handle structure
 * @param data Pointer to store received byte
 * @return MIL_UART_OK if byte read, error code if buffer empty
 */
MIL_UART_ErrorTypeDef MIL_UART_ReadByte(MIL_UART_HandleTypeDef* uart, uint8_t* data);

/**
 * @brief UART interrupt handler (must be called from UART IRQ)
 *
 * Handles TX and RX interrupts for interrupt-driven operation.
 * User must call this from UART1_IRQHandler or UART2_IRQHandler.
 *
 * @param uart Pointer to UART handle structure
 */
void MIL_UART_IRQHandler(MIL_UART_HandleTypeDef* uart);

#endif /* MIL_UART_H */
