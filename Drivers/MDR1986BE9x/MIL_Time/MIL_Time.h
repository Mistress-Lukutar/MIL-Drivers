/**
 * @file MIL_Time.h
 * @brief SysTick driver for 1986BE9x series MCUs
 * @author Mistress-Lukutar
 * @date 2026-07-29
 * @version v1.1.1
 *
 * @note This module provides functions to retrieve system uptime in
 *       milliseconds and microseconds, plus busy-wait delays that keep
 *       working with interrupts disabled (hardware counter polling).
 */

#ifndef MIL_TIME_H
#define MIL_TIME_H

#include "MDR32Fx.h" // Device header
#include <stdint.h>

/* Default system clock if not defined externally (same convention as MIL_eeprom.h) */
#ifndef SYSTEM_CORE_CLOCK_MHZ
#define SYSTEM_CORE_CLOCK_MHZ 80UL /**< Default core clock frequency (MHz) */
#endif

#define MIL_TIME_CYCLES_PER_US SYSTEM_CORE_CLOCK_MHZ /**< CPU cycles per microsecond */

/**
 * @brief Initializes SysTick to generate 1ms interrupts.
 */
void MIL_TIME_Init(void);

/**
 * @brief Returns the elapsed time in milliseconds since system start.
 * @return uint32_t - Milliseconds elapsed since power-up.
 */
uint32_t MIL_TIME_GetMillis(void);

/**
 * @brief Returns the elapsed time in microseconds since system start.
 * @return uint32_t - Microseconds elapsed since power-up.
 * @note Wraps around every ~71.6 minutes (2^32 us); use unsigned subtraction
 *       for interval measurements. Requires SysTick to be running.
 */
uint32_t MIL_TIME_GetMicros(void);

/**
 * @brief Blocks execution for a specified number of microseconds.
 * @param delay_us - Time in microseconds to delay.
 * @note Polls the SysTick hardware counter, so it stays accurate with
 *       interrupts disabled and is immune to compiler optimization level.
 *       If SysTick is not running (early boot), falls back to a conservative
 *       NOP loop that may over-delay but never under-delays.
 */
void MIL_TIME_DelayUs(uint32_t delay_us);

/**
 * @brief Blocks execution for a specified number of milliseconds.
 * @param delay_ms - Time in milliseconds to delay.
 */
void MIL_TIME_Delay(uint32_t delay_ms);

#endif /* MIL_TIME_H */
