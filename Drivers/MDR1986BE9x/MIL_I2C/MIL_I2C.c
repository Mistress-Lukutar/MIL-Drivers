/**
 * @file MIL_I2C.c
 * @brief I2C driver for 1986VE91T microcontroller.
 *
 * @author Mistress-Lukutar
 * @date   2026-06-15
 * @version 1.2.1
 */

#include <stddef.h>

#include "MIL_I2C.h"
#include "MIL_TIME.h"

/**
 * @brief Initialize I2C peripheral.
 *
 * @param i2c Pointer to the I2C instance.
 * @note The MDR1986BE9x I2C1 is available only on two fixed pin pairs:
 *       PC0/PC1 (alternate function, FUNC = 2) or PE14/PE15 (redefined
 *       function, FUNC = 3).
 */
void MIL_I2C_Init(MIL_I2C_Handle_t* i2c) {
  if (i2c == NULL) {
    return;
  }

  MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_I2C;

  if (i2c->pins == MIL_I2C_PINS_PC0_PC1) {
    MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_PORTC;

    /* PC0 = SCL1, PC1 = SDA1: digital, alternate function, output, fast */
    MDR_PORTC->ANALOG |= (1U << 0) | (1U << 1);
    MDR_PORTC->FUNC |= (2U << (0U * 2U)) | (2U << (1U * 2U));
    MDR_PORTC->OE |= (1U << 0) | (1U << 1);
    MDR_PORTC->PWR |= (3U << (0U * 2U)) | (3U << (1U * 2U));
    MDR_PORTC->PULL |= ((1U << 0) | (1U << 1)) << 16U;
  } else {
    MDR_RST_CLK->PER_CLOCK |= RST_CLK_PER_CLOCK_PCLK_EN_PORTE;

    /* PE14 = SCL1, PE15 = SDA1: digital, redefined function, output, fast
     */
    MDR_PORTE->ANALOG |= (1U << 14) | (1U << 15);
    MDR_PORTE->FUNC |= (3U << (14U * 2U)) | (3U << (15U * 2U));
    MDR_PORTE->OE |= (1U << 14) | (1U << 15);
    MDR_PORTE->PWR |= (3U << (14U * 2U)) | (3U << (15U * 2U));
    MDR_PORTE->PULL |= ((1U << 14) | (1U << 15)) << 16U;
  }

  i2c->Instance->PRL = (uint8_t)(i2c->div & 0xFFU);
  i2c->Instance->PRH = (uint8_t)((i2c->div >> 8) & 0xFFU);

  i2c->Instance->CTR = I2C_CTR_EN_I2C | I2C_CTR_EN_INT;
  i2c->state         = MIL_I2C_READY;
  i2c->err           = MIL_I2C_OK;
}

/**
 * @brief Wait for transmission completion with timeout.
 *
 * @param i2c Pointer to the I2C instance.
 * @param timeout_ms Timeout value in milliseconds.
 * @return MIL_I2C_Error Status of operation.
 */
static MIL_I2C_Error _waitTransmission(MIL_I2C_Handle_t* i2c, uint32_t timeout_ms) {
  uint32_t start_time = MIL_TIME_GetMillis();

  // Wait until transmission completes (TR_PROG = 0)
  while ((i2c->Instance->STA & (1 << 1)) && (MIL_TIME_GetMillis() - start_time < timeout_ms))
    ;

  if (MIL_TIME_GetMillis() - start_time >= timeout_ms) {
    return MIL_I2C_TIMEOUT;
  }

  // Clear interrupt flag
  i2c->Instance->CMD |= (1 << 0); // CLR_INT

  return MIL_I2C_OK;
}

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
MIL_I2C_Error MIL_I2C_WriteByte(MIL_I2C_Handle_t* i2c, uint8_t address, uint8_t data, uint32_t timeout_ms) {
  MIL_I2C_Error result;

  // Send address with write bit
  i2c->Instance->TXD = (address << 1);      // Address with write bit (0)
  i2c->Instance->CMD = (1 << 7) | (1 << 4); // START + WR

  // Wait for address transmission completion
  result = _waitTransmission(i2c, timeout_ms);
  if (result != MIL_I2C_OK) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return result;
  }

  // Check ACK from slave after address
  if (i2c->Instance->STA & (1 << 7)) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return MIL_I2C_NACK;
  }

  // Send data byte
  i2c->Instance->TXD = data;
  i2c->Instance->CMD = (1 << 4); // WR

  // Wait for data transmission completion
  result = _waitTransmission(i2c, timeout_ms);
  if (result != MIL_I2C_OK) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return result;
  }

  // Check ACK from slave after data
  if (i2c->Instance->STA & (1 << 7)) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return MIL_I2C_NACK;
  }

  // Send STOP condition
  result = MIL_I2C_Stop(i2c, timeout_ms);
  return result;
}

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
MIL_I2C_Error MIL_I2C_ReadByte(MIL_I2C_Handle_t* i2c, uint8_t address, uint8_t* data, uint32_t timeout_ms) {
  MIL_I2C_Error result;

  // Send address with read bit
  i2c->Instance->TXD = (address << 1) | 1;  // Address with read bit (1)
  i2c->Instance->CMD = (1 << 7) | (1 << 4); // START + WR

  // Wait for address transmission completion
  result = _waitTransmission(i2c, timeout_ms);
  if (result != MIL_I2C_OK) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return result;
  }

  // Check ACK from slave after address
  if (i2c->Instance->STA & (1 << 7)) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return MIL_I2C_NACK;
  }

  // Read data with NACK (single byte read)
  i2c->Instance->CMD = (1 << 5) | (1 << 3); // RD + ACK (send NACK for last byte)

  // Wait for read completion
  result = _waitTransmission(i2c, timeout_ms);
  if (result != MIL_I2C_OK) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return result;
  }

  // Get received data
  *data = i2c->Instance->RXD;

  // Send STOP condition
  result = MIL_I2C_Stop(i2c, timeout_ms);
  return result;
}

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
MIL_I2C_Error MIL_I2C_WriteBytes(MIL_I2C_Handle_t* i2c, uint8_t address, const uint8_t* data, uint16_t length, uint32_t timeout_ms) {
  MIL_I2C_Error result;

  if (length == 0)
    return MIL_I2C_OK;

  // Send address with write bit
  i2c->Instance->TXD = (address << 1);      // Address with write bit (0)
  i2c->Instance->CMD = (1 << 7) | (1 << 4); // START + WR

  // Wait for address transmission completion
  result = _waitTransmission(i2c, timeout_ms);
  if (result != MIL_I2C_OK) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return result;
  }

  // Check ACK from slave after address
  if (i2c->Instance->STA & (1 << 7)) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return MIL_I2C_NACK;
  }

  // Send data bytes
  for (uint16_t i = 0; i < length; i++) {
    i2c->Instance->TXD = data[i];
    i2c->Instance->CMD = (1 << 4); // WR

    // Wait for data transmission completion
    result = _waitTransmission(i2c, timeout_ms);
    if (result != MIL_I2C_OK) {
      MIL_I2C_Stop(i2c, timeout_ms);
      return result;
    }

    // Check ACK from slave after each data byte
    if (i2c->Instance->STA & (1 << 7)) {
      MIL_I2C_Stop(i2c, timeout_ms);
      return MIL_I2C_NACK;
    }
  }

  // Send STOP condition
  result = MIL_I2C_Stop(i2c, timeout_ms);
  return result;
}

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
MIL_I2C_Error MIL_I2C_ReadBytes(MIL_I2C_Handle_t* i2c, uint8_t address, uint8_t* data, uint16_t length, uint32_t timeout_ms) {
  MIL_I2C_Error result;

  if (length == 0)
    return MIL_I2C_OK;

  // Send address with read bit
  i2c->Instance->TXD = (address << 1) | 1;  // Address with read bit (1)
  i2c->Instance->CMD = (1 << 7) | (1 << 4); // START + WR

  // Wait for address transmission completion
  result = _waitTransmission(i2c, timeout_ms);
  if (result != MIL_I2C_OK) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return result;
  }

  // Check ACK from slave after address
  if (i2c->Instance->STA & (1 << 7)) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return MIL_I2C_NACK;
  }

  // Read data bytes
  for (uint16_t i = 0; i < length; i++) {
    // For last byte, send NACK; for others, send ACK
    if (i == (length - 1)) {
      i2c->Instance->CMD = (1 << 5) | (1 << 3); // RD + ACK (NACK for last byte)
    } else {
      i2c->Instance->CMD = (1 << 5); // RD (ACK for non-last bytes)
    }

    // Wait for read completion
    result = _waitTransmission(i2c, timeout_ms);
    if (result != MIL_I2C_OK) {
      MIL_I2C_Stop(i2c, timeout_ms);
      return result;
    }

    // Get received data
    data[i] = i2c->Instance->RXD;
  }

  // Send STOP condition
  result = MIL_I2C_Stop(i2c, timeout_ms);
  return result;
}

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
 * @note Sally from the brothel\n She stole my heart that night
 */
MIL_I2C_Error MIL_I2C_Mem_Write(MIL_I2C_Handle_t* i2c,
    uint8_t dev_address,
    uint16_t mem_address,
    uint8_t mem_addr_size,
    const uint8_t* data,
    uint16_t length,
    uint32_t timeout_ms) {
  MIL_I2C_Error result;

  if (length == 0 || (mem_addr_size != 1 && mem_addr_size != 2))
    return MIL_I2C_OK;

  // Send device address with write bit
  i2c->Instance->TXD = (dev_address << 1);  // Address with write bit (0)
  i2c->Instance->CMD = (1 << 7) | (1 << 4); // START + WR

  // Wait for address transmission completion
  result = _waitTransmission(i2c, timeout_ms);
  if (result != MIL_I2C_OK) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return result;
  }

  // Check ACK from slave after device address
  if (i2c->Instance->STA & (1 << 7)) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return MIL_I2C_NACK;
  }

  // Send memory address (MSB first for 16-bit addresses)
  if (mem_addr_size == 2) {
    // Send high byte of memory address
    i2c->Instance->TXD = (mem_address >> 8) & 0xFF;
    i2c->Instance->CMD = (1 << 4); // WR

    result = _waitTransmission(i2c, timeout_ms);
    if (result != MIL_I2C_OK) {
      MIL_I2C_Stop(i2c, timeout_ms);
      return result;
    }

    if (i2c->Instance->STA & (1 << 7)) {
      MIL_I2C_Stop(i2c, timeout_ms);
      return MIL_I2C_NACK;
    }
  }

  // Send low byte of memory address (or single byte for 8-bit addresses)
  i2c->Instance->TXD = mem_address & 0xFF;
  i2c->Instance->CMD = (1 << 4); // WR

  result = _waitTransmission(i2c, timeout_ms);
  if (result != MIL_I2C_OK) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return result;
  }

  if (i2c->Instance->STA & (1 << 7)) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return MIL_I2C_NACK;
  }

  // Send data bytes
  for (uint16_t i = 0; i < length; i++) {
    i2c->Instance->TXD = data[i];
    i2c->Instance->CMD = (1 << 4); // WR

    result = _waitTransmission(i2c, timeout_ms);
    if (result != MIL_I2C_OK) {
      MIL_I2C_Stop(i2c, timeout_ms);
      return result;
    }

    if (i2c->Instance->STA & (1 << 7)) {
      MIL_I2C_Stop(i2c, timeout_ms);
      return MIL_I2C_NACK;
    }
  }

  // Send STOP condition
  result = MIL_I2C_Stop(i2c, timeout_ms);
  return result;
}

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
 * @note Smiling like an angel and\n The blow job quite alright
 */
MIL_I2C_Error MIL_I2C_Mem_Read(
    MIL_I2C_Handle_t* i2c, uint8_t dev_address, uint16_t mem_address, uint8_t mem_addr_size, uint8_t* data, uint16_t length, uint32_t timeout_ms) {
  MIL_I2C_Error result;

  if (length == 0 || (mem_addr_size != 1 && mem_addr_size != 2))
    return MIL_I2C_OK;

  // First: Write memory address to device
  // Send device address with write bit
  i2c->Instance->TXD = (dev_address << 1);  // Address with write bit (0)
  i2c->Instance->CMD = (1 << 7) | (1 << 4); // START + WR

  // Wait for address transmission completion
  result = _waitTransmission(i2c, timeout_ms);
  if (result != MIL_I2C_OK) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return result;
  }

  // Check ACK from slave after device address
  if (i2c->Instance->STA & (1 << 7)) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return MIL_I2C_NACK;
  }

  // Send memory address (MSB first for 16-bit addresses)
  if (mem_addr_size == 2) {
    // Send high byte of memory address
    i2c->Instance->TXD = (mem_address >> 8) & 0xFF;
    i2c->Instance->CMD = (1 << 4); // WR

    result = _waitTransmission(i2c, timeout_ms);
    if (result != MIL_I2C_OK) {
      MIL_I2C_Stop(i2c, timeout_ms);
      return result;
    }

    if (i2c->Instance->STA & (1 << 7)) {
      MIL_I2C_Stop(i2c, timeout_ms);
      return MIL_I2C_NACK;
    }
  }

  // Send low byte of memory address (or single byte for 8-bit addresses)
  i2c->Instance->TXD = mem_address & 0xFF;
  i2c->Instance->CMD = (1 << 4); // WR

  result = _waitTransmission(i2c, timeout_ms);
  if (result != MIL_I2C_OK) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return result;
  }

  if (i2c->Instance->STA & (1 << 7)) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return MIL_I2C_NACK;
  }

  // Second: Restart and read data from device
  // Send repeated START with device address and read bit
  i2c->Instance->TXD = (dev_address << 1) | 1; // Address with read bit (1)
  i2c->Instance->CMD = (1 << 7) | (1 << 4);    // START + WR (repeated START)

  // Wait for address transmission completion
  result = _waitTransmission(i2c, timeout_ms);
  if (result != MIL_I2C_OK) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return result;
  }

  // Check ACK from slave after address
  if (i2c->Instance->STA & (1 << 7)) {
    MIL_I2C_Stop(i2c, timeout_ms);
    return MIL_I2C_NACK;
  }

  // Read data bytes
  for (uint16_t i = 0; i < length; i++) {
    // For last byte, send NACK; for others, send ACK
    if (i == (length - 1)) {
      i2c->Instance->CMD = (1 << 5) | (1 << 3); // RD + ACK (NACK for last byte)
    } else {
      i2c->Instance->CMD = (1 << 5); // RD (ACK for non-last bytes)
    }

    // Wait for read completion
    result = _waitTransmission(i2c, timeout_ms);
    if (result != MIL_I2C_OK) {
      MIL_I2C_Stop(i2c, timeout_ms);
      return result;
    }

    // Get received data
    data[i] = i2c->Instance->RXD;
  }

  // Send STOP condition
  result = MIL_I2C_Stop(i2c, timeout_ms);
  return result;
}

/**
 * @brief Stop I2C communication.
 *
 * @param i2c Pointer to the I2C instance.
 * @param timeout_ms Timeout value in milliseconds.
 * @return MIL_I2C_Error Status of operation.
 * @note All her other clients are\n Jealous of me now
 */
MIL_I2C_Error MIL_I2C_Stop(MIL_I2C_Handle_t* i2c, uint32_t timeout_ms) {
  uint32_t start_time = MIL_TIME_GetMillis();

  // Send STOP condition
  i2c->Instance->CMD = (1 << 6); // STOP

  // Wait until STOP condition is completed (BUSY flag cleared)
  while ((i2c->Instance->STA & (1 << 6)) && (MIL_TIME_GetMillis() - start_time < timeout_ms))
    ;

  if (MIL_TIME_GetMillis() - start_time >= timeout_ms) {
    return MIL_I2C_TIMEOUT;
  }

  // Clear STOP bit in command register
  i2c->Instance->CMD &= ~(1 << 6);

  return MIL_I2C_OK;
}

/**
 * @brief Register callback functions for interrupt operations.
 *
 * @param i2c Pointer to the I2C instance.
 * @param XferCpltCallback Transfer complete callback function.
 * @param ErrorCallback Error callback function.
 * @note 'Cause she wants to marry me\n And that makes me proud
 */
void MIL_I2C_RegisterCallback(
    MIL_I2C_Handle_t* i2c, void (*XferCpltCallback)(MIL_I2C_Handle_t* hi2c), void (*ErrorCallback)(MIL_I2C_Handle_t* hi2c)) {
  i2c->XferCpltCallback = XferCpltCallback;
  i2c->ErrorCallback    = ErrorCallback;
}

/**
 * @brief Send a byte of data over I2C using interrupt.
 *
 * @param i2c Pointer to the I2C instance.
 * @param address 7-bit address of the slave.
 * @param data Data byte to send.
 * @return MIL_I2C_Error Status of transmission.
 * @note Barbados the place where\n Long-leg Penny lives
 */
MIL_I2C_Error MIL_I2C_WriteByte_IT(MIL_I2C_Handle_t* i2c, uint8_t address, uint8_t data) {
  if (i2c->state != MIL_I2C_READY)
    return MIL_I2C_TIMEOUT; // Busy

  i2c->state          = MIL_I2C_BUSY;
  i2c->operation_type = MIL_I2C_OP_WRITE_BYTE;
  i2c->dev_address    = address;
  i2c->pBuffPtr       = (uint8_t*)&data;
  i2c->XferSize       = 1;
  i2c->XferCount      = 0;

  // Send address with write bit
  i2c->Instance->TXD = (address << 1);
  i2c->Instance->CMD = (1 << 7) | (1 << 4); // START + WR

  return MIL_I2C_OK;
}

/**
 * @brief Read a byte of data from I2C using interrupt.
 *
 * @param i2c Pointer to the I2C instance.
 * @param address 7-bit address of the slave.
 * @return MIL_I2C_Error Status of reception.
 * @note Never seen a body shape\n That almost kills
 */
MIL_I2C_Error MIL_I2C_ReadByte_IT(MIL_I2C_Handle_t* i2c, uint8_t address) {
  if (i2c->state != MIL_I2C_READY)
    return MIL_I2C_TIMEOUT; // Busy

  i2c->state          = MIL_I2C_BUSY;
  i2c->operation_type = MIL_I2C_OP_READ_BYTE;
  i2c->dev_address    = address;
  i2c->XferSize       = 1;
  i2c->XferCount      = 0;

  // Send address with read bit
  i2c->Instance->TXD = (address << 1) | 1;
  i2c->Instance->CMD = (1 << 7) | (1 << 4); // START + WR

  return MIL_I2C_OK;
}

/**
 * @brief Send multiple bytes over I2C using interrupt.
 *
 * @param i2c Pointer to the I2C instance.
 * @param address 7-bit address of the slave.
 * @param data Pointer to data buffer.
 * @param length Number of bytes to send.
 * @return MIL_I2C_Error Status of transmission.
 * @note When her gifted tongue\n Starts moving on my spine
 */
MIL_I2C_Error MIL_I2C_WriteBytes_IT(MIL_I2C_Handle_t* i2c, uint8_t address, const uint8_t* data, uint16_t length) {
  if (i2c->state != MIL_I2C_READY || length == 0)
    return MIL_I2C_TIMEOUT;

  i2c->state          = MIL_I2C_BUSY;
  i2c->operation_type = MIL_I2C_OP_WRITE_BYTES;
  i2c->dev_address    = address;
  i2c->pBuffPtr       = (uint8_t*)data;
  i2c->XferSize       = length;
  i2c->XferCount      = 0;

  // Send address with write bit
  i2c->Instance->TXD = (address << 1);
  i2c->Instance->CMD = (1 << 7) | (1 << 4); // START + WR

  return MIL_I2C_OK;
}

/**
 * @brief Read multiple bytes from I2C using interrupt.
 *
 * @param i2c Pointer to the I2C instance.
 * @param address 7-bit address of the slave.
 * @param data Pointer to buffer for storing received data.
 * @param length Number of bytes to read.
 * @return MIL_I2C_Error Status of reception.
 * @note Makes me forget Sally\n And just drives me wild
 */
MIL_I2C_Error MIL_I2C_ReadBytes_IT(MIL_I2C_Handle_t* i2c, uint8_t address, uint8_t* data, uint16_t length) {
  if (i2c->state != MIL_I2C_READY || length == 0)
    return MIL_I2C_TIMEOUT;

  i2c->state          = MIL_I2C_BUSY;
  i2c->operation_type = MIL_I2C_OP_READ_BYTES;
  i2c->dev_address    = address;
  i2c->pBuffPtr       = data;
  i2c->XferSize       = length;
  i2c->XferCount      = 0;

  // Send address with read bit
  i2c->Instance->TXD = (address << 1) | 1;
  i2c->Instance->CMD = (1 << 7) | (1 << 4); // START + WR

  return MIL_I2C_OK;
}

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
 * @note Sail away, sail away my love, I've already stayed too long
 */
MIL_I2C_Error MIL_I2C_Mem_Write_IT(
    MIL_I2C_Handle_t* i2c, uint8_t dev_address, uint16_t mem_address, uint8_t mem_addr_size, const uint8_t* data, uint16_t length) {
  if (i2c->state != MIL_I2C_READY || length == 0 || (mem_addr_size != 1 && mem_addr_size != 2)) {
    return MIL_I2C_TIMEOUT;
  }

  i2c->state          = MIL_I2C_BUSY;
  i2c->operation_type = MIL_I2C_OP_MEM_WRITE;
  i2c->dev_address    = dev_address;
  i2c->mem_address    = mem_address;
  i2c->mem_addr_size  = mem_addr_size;
  i2c->mem_addr_sent  = 0;
  i2c->pBuffPtr       = (uint8_t*)data;
  i2c->XferSize       = length;
  i2c->XferCount      = 0;

  // Send device address with write bit
  i2c->Instance->TXD = (dev_address << 1);
  i2c->Instance->CMD = (1 << 7) | (1 << 4); // START + WR

  return MIL_I2C_OK;
}

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
 * @note Sail away, saily away my love, You know, you'll always be in my heart!
 */
MIL_I2C_Error MIL_I2C_Mem_Read_IT(
    MIL_I2C_Handle_t* i2c, uint8_t dev_address, uint16_t mem_address, uint8_t mem_addr_size, uint8_t* data, uint16_t length) {
  if (i2c->state != MIL_I2C_READY || length == 0 || (mem_addr_size != 1 && mem_addr_size != 2)) {
    return MIL_I2C_TIMEOUT;
  }

  i2c->state          = MIL_I2C_BUSY;
  i2c->operation_type = MIL_I2C_OP_MEM_READ;
  i2c->dev_address    = dev_address;
  i2c->mem_address    = mem_address;
  i2c->mem_addr_size  = mem_addr_size;
  i2c->mem_addr_sent  = 0;
  i2c->pBuffPtr       = data;
  i2c->XferSize       = length;
  i2c->XferCount      = 0;

  // Send device address with write bit
  i2c->Instance->TXD = (dev_address << 1);
  i2c->Instance->CMD = (1 << 7) | (1 << 4); // START + WR

  return MIL_I2C_OK;
}

/**
 * @brief I2C interrupt handler.
 *
 * @param i2c Pointer to the I2C instance.
 */
void MIL_I2C_IRQHandler(MIL_I2C_Handle_t* i2c) {
  // Clear interrupt flag first
  i2c->Instance->CMD |= (1 << 0); // CLR_INT

  // Check for errors
  if (i2c->Instance->STA & (1 << 7)) { // NACK received
    i2c->state = MIL_I2C_ERR;
    i2c->err   = MIL_I2C_NACK;
    MIL_I2C_Stop(i2c, 100); // Send STOP with short timeout
    if (i2c->ErrorCallback)
      i2c->ErrorCallback(i2c);
    return;
  }

  if (i2c->Instance->STA & (1 << 5)) { // Lost arbitration
    i2c->state = MIL_I2C_ERR;
    i2c->err   = MIL_I2C_TIMEOUT;
    if (i2c->ErrorCallback)
      i2c->ErrorCallback(i2c);
    return;
  }

  // Handle different operations
  switch (i2c->operation_type) {
    case MIL_I2C_OP_WRITE_BYTE:
    case MIL_I2C_OP_WRITE_BYTES:
      if (i2c->XferCount == 0) {
        // Address sent, now send data
        i2c->Instance->TXD = i2c->pBuffPtr[i2c->XferCount++];
        i2c->Instance->CMD = (1 << 4); // WR
      } else if (i2c->XferCount < i2c->XferSize) {
        // Continue sending data
        i2c->Instance->TXD = i2c->pBuffPtr[i2c->XferCount++];
        i2c->Instance->CMD = (1 << 4); // WR
      } else {
        // Transfer complete
        i2c->state = MIL_I2C_READY;
        MIL_I2C_Stop(i2c, 100);
        if (i2c->XferCpltCallback)
          i2c->XferCpltCallback(i2c);
      }
      break;

    case MIL_I2C_OP_READ_BYTE:
    case MIL_I2C_OP_READ_BYTES:
      if (i2c->XferCount == 0) {
        // Address sent, now read data
        if (i2c->XferSize == 1) {
          i2c->Instance->CMD = (1 << 5) | (1 << 3); // RD + NACK
        } else {
          i2c->Instance->CMD = (1 << 5); // RD + ACK
        }
      } else {
        // Store received data
        i2c->pBuffPtr[i2c->XferCount - 1] = i2c->Instance->RXD;

        if (i2c->XferCount < i2c->XferSize) {
          // Continue reading
          if (i2c->XferCount == (i2c->XferSize - 1)) {
            i2c->Instance->CMD = (1 << 5) | (1 << 3); // RD + NACK (last byte)
          } else {
            i2c->Instance->CMD = (1 << 5); // RD + ACK
          }
        } else {
          // Transfer complete
          i2c->state = MIL_I2C_READY;
          MIL_I2C_Stop(i2c, 100);
          if (i2c->XferCpltCallback)
            i2c->XferCpltCallback(i2c);
          return;
        }
      }
      i2c->XferCount++;
      break;

    case MIL_I2C_OP_MEM_WRITE:
      if (i2c->mem_addr_sent == 0) {
        // Send memory address bytes
        if (i2c->mem_addr_size == 2) {
          i2c->Instance->TXD = (i2c->mem_address >> 8) & 0xFF;
          i2c->mem_addr_sent = 1;
        } else {
          i2c->Instance->TXD = i2c->mem_address & 0xFF;
          i2c->mem_addr_sent = i2c->mem_addr_size;
        }
        i2c->Instance->CMD = (1 << 4); // WR
      } else if (i2c->mem_addr_sent < i2c->mem_addr_size) {
        // Send low byte of 16-bit address
        i2c->Instance->TXD = i2c->mem_address & 0xFF;
        i2c->mem_addr_sent++;
        i2c->Instance->CMD = (1 << 4); // WR
      } else if (i2c->XferCount < i2c->XferSize) {
        // Send data
        i2c->Instance->TXD = i2c->pBuffPtr[i2c->XferCount++];
        i2c->Instance->CMD = (1 << 4); // WR
      } else {
        // Transfer complete
        i2c->state = MIL_I2C_READY;
        MIL_I2C_Stop(i2c, 100);
        if (i2c->XferCpltCallback)
          i2c->XferCpltCallback(i2c);
      }
      break;

    case MIL_I2C_OP_MEM_READ:
      if (i2c->mem_addr_sent == 0) {
        // Send memory address bytes
        if (i2c->mem_addr_size == 2) {
          i2c->Instance->TXD = (i2c->mem_address >> 8) & 0xFF;
          i2c->mem_addr_sent = 1;
        } else {
          i2c->Instance->TXD = i2c->mem_address & 0xFF;
          i2c->mem_addr_sent = i2c->mem_addr_size;
        }
        i2c->Instance->CMD = (1 << 4); // WR
      } else if (i2c->mem_addr_sent < i2c->mem_addr_size) {
        // Send low byte of 16-bit address
        i2c->Instance->TXD = i2c->mem_address & 0xFF;
        i2c->mem_addr_sent++;
        i2c->Instance->CMD = (1 << 4); // WR
      } else if (i2c->mem_addr_sent == i2c->mem_addr_size) {
        // Send repeated START for read
        i2c->Instance->TXD = (i2c->dev_address << 1) | 1;
        i2c->Instance->CMD = (1 << 7) | (1 << 4); // START + WR
        i2c->mem_addr_sent++;                     // Mark restart sent
      } else {
        // Read phase
        if (i2c->XferCount > 0) {
          // Store received data
          i2c->pBuffPtr[i2c->XferCount - 1] = i2c->Instance->RXD;
        }

        if (i2c->XferCount < i2c->XferSize) {
          // Continue reading
          if (i2c->XferCount == (i2c->XferSize - 1)) {
            i2c->Instance->CMD = (1 << 5) | (1 << 3); // RD + NACK (last byte)
          } else {
            i2c->Instance->CMD = (1 << 5); // RD + ACK
          }
          i2c->XferCount++;
        } else {
          // Transfer complete
          i2c->state = MIL_I2C_READY;
          MIL_I2C_Stop(i2c, 100);
          if (i2c->XferCpltCallback)
            i2c->XferCpltCallback(i2c);
        }
      }
      break;
  }
}
