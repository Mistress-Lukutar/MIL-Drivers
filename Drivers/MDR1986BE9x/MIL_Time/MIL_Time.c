/**
 * @file MIL_Time.c
 * @brief Implementation of millisecond/microsecond timebase using SysTick
 * @author Mistress-Lukutar
 * @date 2026-07-29
 * @version v1.1.1
 *
 * @note This module provides functions to retrieve the system uptime in
 *       milliseconds and microseconds, plus busy-wait delays based on polling
 *       the SysTick hardware counter (usable with interrupts disabled).
 */

#include "MIL_Time.h"

static volatile uint32_t millis_counter = 0;

/**
 * @brief SysTick interrupt handler.
 * Increments the millisecond counter.
 */
void SysTick_Handler(void) { millis_counter++; }

/**
 * @brief Initializes SysTick to generate 1ms interrupts.
 */
void MIL_TIME_Init(void) {
  SysTick->LOAD = MIL_TIME_CYCLES_PER_US * 1000U - 1U;             // CPU cycles per 1 ms
  SysTick->VAL  = 0U;                                              // Reset counter value
  NVIC_SetPriority(SysTick_IRQn, (1UL << __NVIC_PRIO_BITS) - 1UL); // Set Priority for Systick Interrupt
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

/**
 * @brief Returns the elapsed time in milliseconds since system start.
 * @return uint32_t - Milliseconds elapsed since power-up.
 */
uint32_t MIL_TIME_GetMillis(void) { return millis_counter; }

/**
 * @brief Returns the elapsed time in microseconds since system start.
 * @return uint32_t - Microseconds elapsed since power-up.
 */
uint32_t MIL_TIME_GetMicros(void) {
  uint32_t ms;
  uint32_t val;

  /* Re-read if the millisecond counter ticked between the two reads */
  do {
    ms  = millis_counter;
    val = SysTick->VAL;
  } while (ms != millis_counter);

  return ms * 1000U + (SysTick->LOAD - val) / MIL_TIME_CYCLES_PER_US;
}

/**
 * @brief Blocks execution for a specified number of microseconds.
 * @param delay_us - Time in microseconds to delay.
 */
void MIL_TIME_DelayUs(uint32_t delay_us) {
  if (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk) {
    /* Hardware timebase: poll the free-running down-counter. Works with
     * interrupts disabled (PRIMASK does not stop SysTick) and cannot be
     * distorted by compiler optimization of a software loop. */
    const uint32_t target_cycles = delay_us * MIL_TIME_CYCLES_PER_US;
    const uint32_t reload        = SysTick->LOAD + 1U;
    uint32_t prev                = SysTick->VAL;
    uint32_t elapsed             = 0U;

    while (elapsed < target_cycles) {
      const uint32_t now = SysTick->VAL;
      /* VAL counts down; on wrap the remaining span is prev + (reload - now) */
      elapsed += (now <= prev) ? (prev - now) : (prev + reload - now);
      prev = now;
    }
  } else {
    /* Fallback for early boot (SysTick not started): conservative NOP loop.
     * The volatile counter forces a load/store per iteration, so the real
     * cost is >= the assumed 4 cycles even at -O3: the delay may overshoot
     * but never undershoots (safe direction for hardware timings). */
    volatile uint32_t count = delay_us * (MIL_TIME_CYCLES_PER_US / 4U);
    if (count == 0U) {
      count = 1U;
    }
    while (count > 0U) {
      count--;
      __NOP();
    }
  }
}

/**
 * @brief Blocks execution for a specified number of milliseconds.
 * @param delay_ms - Time in milliseconds to delay.
 */
void MIL_TIME_Delay(uint32_t delay_ms) {
  uint32_t start = MIL_TIME_GetMillis();
  while (MIL_TIME_GetMillis() - start < delay_ms) { }
}
