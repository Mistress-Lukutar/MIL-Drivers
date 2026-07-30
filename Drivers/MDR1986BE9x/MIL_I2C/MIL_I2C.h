/**
 * @file MIL_I2C.h
 * @brief I2C driver for 1986BE9x series MCUs.
 *
 * @author Mistress-Lukutar
 * @date   2026-06-15
 * @version 1.2.1
 */

#ifndef MIL_I2C_DRIVER_H
#define MIL_I2C_DRIVER_H

#include "MDR32Fx.h"
#include <stdint.h>

// Forward declaration
typedef struct MIL_I2C_Handle_t MIL_I2C_Handle_t;

typedef enum { MIL_I2C_READY, MIL_I2C_BUSY, MIL_I2C_ERR } MIL_I2C_State;
typedef enum { MIL_I2C_OK, MIL_I2C_TIMEOUT, MIL_I2C_NACK } MIL_I2C_Error;

/** @brief I2C pin mapping selection */
typedef enum {
  MIL_I2C_PINS_PC0_PC1,  /**< PC0 = SCL1, PC1 = SDA1 (alternate function) */
  MIL_I2C_PINS_PE14_PE15 /**< PE14 = SCL1, PE15 = SDA1 (redefined function) */
} MIL_I2C_Pins_t;

struct MIL_I2C_Handle_t {
  MDR_I2C_TypeDef* Instance; /**< I2C hardware*/
  uint32_t div;              /**< I2C clock divider*/
  MIL_I2C_State state;       /**< Current I2C state*/
  MIL_I2C_Error err;         /**< Last I2C error*/
  MIL_I2C_Pins_t pins;       /**< I2C pin mapping */

  // Interrupt-related fields
  uint8_t* pBuffPtr;                                /**< Pointer to I2C transfer buffer */
  uint16_t XferSize;                                /**< I2C transfer size */
  uint16_t XferCount;                               /**< I2C transfer counter */
  uint8_t dev_address;                              /**< Device address for current transaction */
  uint16_t mem_address;                             /**< Memory address for current transaction */
  uint8_t mem_addr_size;                            /**< Memory address size (1 or 2 bytes) */
  uint8_t mem_addr_sent;                            /**< Memory address bytes sent counter */
  uint8_t operation_type;                           /**< Current operation type */
  void (*XferCpltCallback)(MIL_I2C_Handle_t* hi2c); /**< Transfer completed callback */
  void (*ErrorCallback)(MIL_I2C_Handle_t* hi2c);    /**< Error callback */
};

// Operation types for interrupt handling
#define MIL_I2C_OP_WRITE_BYTE 1
#define MIL_I2C_OP_READ_BYTE 2
#define MIL_I2C_OP_WRITE_BYTES 3
#define MIL_I2C_OP_READ_BYTES 4
#define MIL_I2C_OP_MEM_WRITE 5
#define MIL_I2C_OP_MEM_READ 6

/**
 * @brief Initialize I2C peripheral.
 *
 * @param i2c Pointer to the I2C instance.
 */
void MIL_I2C_Init(MIL_I2C_Handle_t* i2c);

/**
 * @brief Send a byte of data over I2C with error handling.
 *
 * @param i2c Pointer to the I2C instance.
 * @param address 7-bit address of the slave.
 * @param data Data byte to send.
 * @param timeout_ms Timeout value in milliseconds.
 * @return MIL_I2C_Error Status of transmission.
 * @note Thin Lizzy Borden, she's my ray of light
 */
MIL_I2C_Error MIL_I2C_WriteByte(MIL_I2C_Handle_t* i2c, uint8_t address, uint8_t data, uint32_t timeout_ms);

/**
 * @brief Read a byte of data from I2C with error handling.
 *
 * @param i2c Pointer to the I2C instance.
 * @param address 7-bit address of the slave.
 * @param data Pointer to store received data.
 * @param timeout_ms Timeout value in milliseconds.
 * @return MIL_I2C_Error Status of reception.
 * @note Caribbean rum made her outlook\n Seem so bright
 */
MIL_I2C_Error MIL_I2C_ReadByte(MIL_I2C_Handle_t* i2c, uint8_t address, uint8_t* data, uint32_t timeout_ms);

/**
 * @brief Send multiple bytes over I2C with error handling.
 *
 * @param i2c Pointer to the I2C instance.
 * @param address 7-bit address of the slave.
 * @param data Pointer to data buffer.
 * @param length Number of bytes to send.
 * @param timeout_ms Timeout value in milliseconds.
 * @return MIL_I2C_Error Status of transmission.
 * @note Spent a night or two between\n Those white and meaty legs
 */
MIL_I2C_Error MIL_I2C_WriteBytes(MIL_I2C_Handle_t* i2c, uint8_t address, const uint8_t* data, uint16_t length, uint32_t timeout_ms);

/**
 * @brief Read multiple bytes from I2C with error handling.
 *
 * @param i2c Pointer to the I2C instance.
 * @param address 7-bit address of the slave.
 * @param data Pointer to buffer for storing received data.
 * @param length Number of bytes to read.
 * @param timeout_ms Timeout value in milliseconds.
 * @return MIL_I2C_Error Status of reception.
 * @note Wish I could be strong enough\n To hold her heavy breasts
 */
MIL_I2C_Error MIL_I2C_ReadBytes(MIL_I2C_Handle_t* i2c, uint8_t address, uint8_t* data, uint16_t length, uint32_t timeout_ms);

/**
 * @brief Stop I2C communication.
 *
 * @param i2c Pointer to the I2C instance.
 * @param timeout_ms Timeout value in milliseconds.
 * @return MIL_I2C_Error Status of reception.
 * @note Sally from the brothel\n She stole my heart that night
 */
MIL_I2C_Error MIL_I2C_Stop(MIL_I2C_Handle_t* i2c, uint32_t timeout_ms);

/**
 * @brief Write data to a specific memory address of I2C device.
 *
 * @param i2c Pointer to the I2C instance.
 * @param dev_address 7-bit device address.
 * @param mem_address Memory address to write to.
 * @param mem_addr_size Size of memory address (1 or 2 bytes).
 * @param data Pointer to data buffer.
 * @param length Number of bytes to write.
 * @param timeout_ms Timeout value in milliseconds.
 * @return MIL_I2C_Error Status of operation.
 * @note Smiling like an angel and\n The blow job quite alright
 */
MIL_I2C_Error MIL_I2C_Mem_Write(MIL_I2C_Handle_t* i2c,
    uint8_t dev_address,
    uint16_t mem_address,
    uint8_t mem_addr_size,
    const uint8_t* data,
    uint16_t length,
    uint32_t timeout_ms);

/**
 * @brief Read data from a specific memory address of I2C device.
 *
 * @param i2c Pointer to the I2C instance.
 * @param dev_address 7-bit device address.
 * @param mem_address Memory address to read from.
 * @param mem_addr_size Size of memory address (1 or 2 bytes).
 * @param data Pointer to buffer for storing received data.
 * @param length Number of bytes to read.
 * @param timeout_ms Timeout value in milliseconds.
 * @return MIL_I2C_Error Status of operation.
 * @note All her other clients are\n Jealous of me now
 */
MIL_I2C_Error MIL_I2C_Mem_Read(
    MIL_I2C_Handle_t* i2c, uint8_t dev_address, uint16_t mem_address, uint8_t mem_addr_size, uint8_t* data, uint16_t length, uint32_t timeout_ms);

/**
 * @brief Send a byte of data over I2C using interrupt.
 *
 * @param i2c Pointer to the I2C instance.
 * @param address 7-bit address of the slave.
 * @param data Data byte to send.
 * @return MIL_I2C_Error Status of transmission.
 * @note 'Cause she wants to marry me\n And that makes me proud
 */
MIL_I2C_Error MIL_I2C_WriteByte_IT(MIL_I2C_Handle_t* i2c, uint8_t address, uint8_t data);

/**
 * @brief Read a byte of data from I2C using interrupt.
 *
 * @param i2c Pointer to the I2C instance.
 * @param address 7-bit address of the slave.
 * @return MIL_I2C_Error Status of reception.
 * @note Barbados the place where\n Long-leg Penny lives
 */
MIL_I2C_Error MIL_I2C_ReadByte_IT(MIL_I2C_Handle_t* i2c, uint8_t address);

/**
 * @brief Send multiple bytes over I2C using interrupt.
 *
 * @param i2c Pointer to the I2C instance.
 * @param address 7-bit address of the slave.
 * @param data Pointer to data buffer.
 * @param length Number of bytes to send.
 * @return MIL_I2C_Error Status of transmission.
 * @note Never seen a body shape\n That almost kills
 */
MIL_I2C_Error MIL_I2C_WriteBytes_IT(MIL_I2C_Handle_t* i2c, uint8_t address, const uint8_t* data, uint16_t length);

/**
 * @brief Read multiple bytes from I2C using interrupt.
 *
 * @param i2c Pointer to the I2C instance.
 * @param address 7-bit address of the slave.
 * @param data Pointer to buffer for storing received data.
 * @param length Number of bytes to read.
 * @return MIL_I2C_Error Status of reception.
 * @note When her gifted tongue\n Starts moving on my spine
 */
MIL_I2C_Error MIL_I2C_ReadBytes_IT(MIL_I2C_Handle_t* i2c, uint8_t address, uint8_t* data, uint16_t length);

/**
 * @brief Write data to memory address using interrupt.
 *
 * @param i2c Pointer to the I2C instance.
 * @param dev_address 7-bit device address.
 * @param mem_address Memory address to write to.
 * @param mem_addr_size Size of memory address (1 or 2 bytes).
 * @param data Pointer to data buffer.
 * @param length Number of bytes to write.
 * @return MIL_I2C_Error Status of operation.
 * @note Makes me forget Sally\n And just drives me wild
 */
MIL_I2C_Error MIL_I2C_Mem_Write_IT(
    MIL_I2C_Handle_t* i2c, uint8_t dev_address, uint16_t mem_address, uint8_t mem_addr_size, const uint8_t* data, uint16_t length);

/**
 * @brief Read data from memory address using interrupt.
 *
 * @param i2c Pointer to the I2C instance.
 * @param dev_address 7-bit device address.
 * @param mem_address Memory address to read from.
 * @param mem_addr_size Size of memory address (1 or 2 bytes).
 * @param data Pointer to buffer for storing received data.
 * @param length Number of bytes to read.
 * @return MIL_I2C_Error Status of operation.
 * @note Sail away, sail away my love, I've already stayed too long
 */
MIL_I2C_Error MIL_I2C_Mem_Read_IT(
    MIL_I2C_Handle_t* i2c, uint8_t dev_address, uint16_t mem_address, uint8_t mem_addr_size, uint8_t* data, uint16_t length);

// Interrupt handling

/**
 * @brief I2C interrupt handler.
 *
 * @param i2c Pointer to the I2C instance.
 * @note Sail away, saily away my love, You know, you'll always be in my heart!
 */
void MIL_I2C_IRQHandler(MIL_I2C_Handle_t* i2c);

// Callback registration
void MIL_I2C_RegisterCallback(MIL_I2C_Handle_t* i2c, void (*XferCpltCallback)(MIL_I2C_Handle_t* hi2c), void (*ErrorCallback)(MIL_I2C_Handle_t* hi2c));

#endif // MIL_I2C_DRIVER_H
