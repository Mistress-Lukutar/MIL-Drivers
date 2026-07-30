/**
 * @file MIL_ADC.h
 * @brief Library for Milandr 1986VE9x ADC peripheral control
 * @author Mistress-Lukutar
 * @date 2025-10-13
 * @version v1.0.0
 */

#ifndef MIL_ADC_H
#define MIL_ADC_H

#include "MDR32Fx.h"
#include <stdint.h>
#include <stdlib.h>

/* ========================================================================== */
/*                           SPECIAL CHANNEL NUMBERS                          */
/* ========================================================================== */

/** @brief Internal voltage reference channel number */
#define MIL_ADC_CHANNEL_VREF ADC_CFG_REG_CHS_VREF
/** @brief Temperature sensor channel number */
#define MIL_ADC_CHANNEL_TEMP ADC_CFG_REG_CHS_TS

/* ========================================================================== */
/*                            TYPE DEFINITIONS                                */
/* ========================================================================== */

/**
 * @brief ADC number enumeration
 */
typedef enum {
  MIL_ADC1 = 0, /**< ADC1 */
  MIL_ADC2 = 1  /**< ADC2 */
} MIL_ADC_Number;

/**
 * @brief ADC clock source enumeration
 */
typedef enum {
  MIL_ADC_CLOCK_CPU = 0, /**< Use CPU clock */
  MIL_ADC_CLOCK_ADC = 1  /**< Use ADC clock */
} MIL_ADC_ClockSource;

/**
 * @brief ADC reference voltage source enumeration
 */
typedef enum {
  MIL_ADC_REF_INTERNAL = 0, /**< Internal reference (AUCC/AGND) */
  MIL_ADC_REF_EXTERNAL = 1  /**< External reference (ADC0_REF+/ADC1_REF-) */
} MIL_ADC_RefSource;

/**
 * @brief ADC conversion mode enumeration
 */
typedef enum {
  MIL_ADC_MODE_SINGLE     = 0, /**< Single conversion */
  MIL_ADC_MODE_CONTINUOUS = 1  /**< Continuous conversion */
} MIL_ADC_ConversionMode;

/**
 * @brief ADC configuration structure
 */
typedef struct {
  MIL_ADC_Number adc_number;        /**< ADC number (ADC1 or ADC2) */
  MIL_ADC_ClockSource clock_source; /**< Clock source selection */
  uint8_t clock_divider;            /**< Clock divider (0-11) */
  MIL_ADC_RefSource ref_source;     /**< Reference voltage source */
  MIL_ADC_ConversionMode conv_mode; /**< Conversion mode */
  uint8_t delay_go;                 /**< Delay before conversion (0-7) */
  uint8_t channel;                  /**< Channel number (0-31) */
  uint8_t enable_ts;                /**< Enable temperature sensor */
  uint8_t enable_ts_buffer;         /**< Enable TS buffer */
  uint8_t use_ts_vref;              /**< Use TS voltage reference */
  uint8_t vref_trim;                /**< Voltage reference trim (0-15) */
} MIL_ADC_Config;

/**
 * @brief ADC handle structure
 */
typedef struct {
  volatile uint32_t* cfg_reg;     /**< Configuration register pointer */
  volatile uint32_t* h_level_reg; /**< Upper level register pointer */
  volatile uint32_t* l_level_reg; /**< Lower level register pointer */
  volatile uint32_t* result_reg;  /**< Result register pointer */
  volatile uint32_t* status_reg;  /**< Status register pointer */
  volatile uint32_t* chsel_reg;   /**< Channel select register pointer */
  MIL_ADC_Config config;          /**< ADC configuration */
} MIL_ADC_Handle;

/**
 * @brief ADC conversion result structure
 */
typedef struct {
  uint16_t value;  /**< Conversion result value (12-bit) */
  uint8_t channel; /**< Channel number that was converted */
} MIL_ADC_Result;

/* ========================================================================== */
/*                          FUNCTION PROTOTYPES                               */
/* ========================================================================== */

/**
 * @brief Initialize ADC handle and registers
 * @param handle Pointer to ADC handle structure
 * @param config Pointer to ADC configuration structure
 * @return 1 on success, 0 on failure
 */
uint8_t MIL_ADC_Init(MIL_ADC_Handle* handle, MIL_ADC_Config* config);

/**
 * @brief Enable ADC peripheral
 * @param handle Pointer to ADC handle structure
 */
void MIL_ADC_Enable(MIL_ADC_Handle* handle);

/**
 * @brief Disable ADC peripheral
 * @param handle Pointer to ADC handle structure
 */
void MIL_ADC_Disable(MIL_ADC_Handle* handle);

/**
 * @brief Start ADC conversion
 * @param handle Pointer to ADC handle structure
 */
void MIL_ADC_StartConversion(MIL_ADC_Handle* handle);

/**
 * @brief Check if conversion is complete
 * @param handle Pointer to ADC handle structure
 * @return 1 if conversion complete, 0 otherwise
 */
uint8_t MIL_ADC_IsConversionComplete(MIL_ADC_Handle* handle);

/**
 * @brief Get ADC conversion result
 * @param handle Pointer to ADC handle structure
 * @param result Pointer to result structure to store data
 * @return 1 on success, 0 on failure
 */
uint8_t MIL_ADC_GetResult(MIL_ADC_Handle* handle, MIL_ADC_Result* result);

/**
 * @brief Set ADC channel for conversion
 * @param handle Pointer to ADC handle structure
 * @param channel Channel number (0-31)
 */
void MIL_ADC_SetChannel(MIL_ADC_Handle* handle, uint8_t channel);

/**
 * @brief Configure multi-channel scanning
 * @param handle Pointer to ADC handle structure
 * @param channel_mask Bit mask of channels to scan
 * @param enable Enable (1) or disable (0) channel switching
 */
void MIL_ADC_ConfigureChannelScan(MIL_ADC_Handle* handle, uint32_t channel_mask, uint8_t enable);

/**
 * @brief Set boundary levels for range checking
 * @param handle Pointer to ADC handle structure
 * @param lower_level Lower boundary (12-bit value)
 * @param upper_level Upper boundary (12-bit value)
 * @param enable Enable (1) or disable (0) range checking
 */
void MIL_ADC_SetBoundaryLevels(MIL_ADC_Handle* handle, uint16_t lower_level, uint16_t upper_level, uint8_t enable);

/**
 * @brief Check if result is out of range
 * @param handle Pointer to ADC handle structure
 * @return 1 if out of range, 0 otherwise
 */
uint8_t MIL_ADC_IsOutOfRange(MIL_ADC_Handle* handle);

/**
 * @brief Check if result was overwritten
 * @param handle Pointer to ADC handle structure
 * @return 1 if overwritten, 0 otherwise
 */
uint8_t MIL_ADC_IsOverwritten(MIL_ADC_Handle* handle);

/**
 * @brief Clear status flags
 * @param handle Pointer to ADC handle structure
 * @param flags Flags to clear (use ADC_STATUS_* defines)
 */
void MIL_ADC_ClearFlags(MIL_ADC_Handle* handle, uint32_t flags);

/**
 * @brief Enable ADC interrupts
 * @param handle Pointer to ADC handle structure
 * @param eoc_int Enable end-of-conversion interrupt
 * @param awoi_int Enable out-of-range interrupt
 */
void MIL_ADC_EnableInterrupts(MIL_ADC_Handle* handle, uint8_t eoc_int, uint8_t awoi_int);

/**
 * @brief Disable ADC interrupts
 * @param handle Pointer to ADC handle structure
 * @param eoc_int Disable end-of-conversion interrupt
 * @param awoi_int Disable out-of-range interrupt
 */
void MIL_ADC_DisableInterrupts(MIL_ADC_Handle* handle, uint8_t eoc_int, uint8_t awoi_int);

/**
 * @brief Configure synchronous dual ADC mode
 * @param handle1 Pointer to ADC1 handle structure
 * @param handle2 Pointer to ADC2 handle structure
 * @param delay_adc Delay between ADC1 and ADC2 start (0-15)
 * @param enable Enable (1) or disable (0) synchronous mode
 */
void MIL_ADC_ConfigureSyncMode(MIL_ADC_Handle* handle1, MIL_ADC_Handle* handle2, uint8_t delay_adc, uint8_t enable);

/**
 * @brief Read temperature sensor
 * @param handle Pointer to ADC1 handle structure
 * @param result Pointer to result structure to store data
 * @return 1 on success, 0 on failure
 */
uint8_t MIL_ADC_ReadTemperature(MIL_ADC_Handle* handle, MIL_ADC_Result* result);

/**
 * @brief Read internal voltage reference
 * @param handle Pointer to ADC1 handle structure
 * @param result Pointer to result structure to store data
 * @return 1 on success, 0 on failure
 */
uint8_t MIL_ADC_ReadVoltageRef(MIL_ADC_Handle* handle, MIL_ADC_Result* result);

/**
 * @brief Perform single channel conversion (blocking)
 * @param handle Pointer to ADC handle structure
 * @param channel Channel number (0-31)
 * @param result Pointer to result structure to store data
 * @param timeout Timeout in arbitrary units
 * @return 1 on success, 0 on timeout
 */
uint8_t MIL_ADC_ConvertChannel(MIL_ADC_Handle* handle, uint8_t channel, MIL_ADC_Result* result, uint32_t timeout);

/**
 * @brief Set delay between channel conversions in scan mode
 * @param handle Pointer to ADC handle structure (ADC2 only)
 * @param delay Delay in CPU_CLK cycles (0-7: 1-8 cycles)
 */
void MIL_ADC_SetScanDelay(MIL_ADC_Handle* handle, uint8_t delay);

/**
 * @brief Enable ADC interrupt in NVIC
 */
void MIL_ADC_EnableNVIC(void);

/**
 * @brief Disable ADC interrupt in NVIC
 */
void MIL_ADC_DisableNVIC(void);

#endif /* MIL_ADC_H */
