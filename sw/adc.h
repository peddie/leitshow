/**
 \file adc.h
 \brief Analog to Digital conversion for SuperConductor
 \defgroup adc_driver ADC
 \ingroup driver

 \copyright
 COSMOGIA PROPRIETARY
 (c) Cosmogia 2013
 All Rights Reserved.
 NOTICE:  All information contained herein is, and remains
 the property of Cosmogia Incorporated and its suppliers,
 if any.  The intellectual and technical concepts contained
 herein are proprietary to Cosmogia Incorporated
 and its suppliers and may be covered by U.S. and Foreign Patents,
 patents in process, and are protected by trade secret or copyright law.
 Dissemination of this information or reproduction of this material
 is strictly forbidden unless prior written permission is obtained
 from Cosmogia Incorporated.

 \section adc_usage Usage
 To use the ADC, do the following:
  - Call adc_setup() from boardsupport.h to enable the relevant clocks and associated DMA
  - Call adc_init() to configure the device
  - Call adc_poll() to update the the ADC readings
  - Results are available in the global buffer: \ref GLOBAL_ACD_DMA_BUFFER

 \section adc_build6 Build 6 Sensor List
 This information comes from the following sources:
  - Conductor board schematic, copied on Feb 1, 2013.
  - STM Datasheet
  - STM Manual

\subsection adc1_inputs ADC1 Inputs
 Sensor     |  #    | Pin Name  | ADC Input
 -------    | ----- | --------- | ---------
 MAG_X_I    | 56    | PB0     | ADC12_IN8
 MAG_Y_I    | 57    | PB1     | ADC12_IN9
 MAG_Z_I    | 54  | PC4   | ADC12_IN14
 Blank      | 55  | PC5   | ADC12_1N15
 Temperature  | xx  | xx    | ADC1_IN16
 Vbat     | xx  | xx    | ADC1_IN17

\subsection adc3_inputs ADC3 Inputs
 Sensor     |  #    | Pin Name  | ADC Input
 -------    | ----- | --------- | ---------
 COND_I     | 19  | PF3   | ADC3_IN9
 SENS_I     | 20  | PF4   | ADC3_IN14
 VCC_5_I    | 21  | PF5   | ADC3_IN15
 BAT_IRID_V_DIV | 22  | PF6   | ADC3_IN4
 BAT_CPU_V_DIV  | 23  | PF7   | ADC3_IN5
 VCC_8_DIV    | 24  | PF8   | ADC3_IN6
 ACS_I      | 25  | PF9   | ADC3_IN7


 -------------------------------------------------------------------------*/

#ifndef __SUPERCONDUCTOR_ADC_H__
#define __SUPERCONDUCTOR_ADC_H__

/*! \addtogroup adc_driver
  @{
*/
#include <stdint.h>
#include <stdbool.h>

typedef enum {
  MAG_X_I        = 0,
  MAG_Y_I        = 1,
  MAG_Z_I        = 2,
  Blank          = 3,
  Temperature    = 4,
  Vbat           = 5,
  COND_I         = 8,
  SENS_I         = 9,
  VCC_5_I        = 10,
  BAT_IRID_V_DIV = 11,
  BAT_CPU_V_DIV  = 12,
  VCC_8_DIV      = 13,
  ACS_I          = 14
} adc_fields_e;

// Prototypes
void adc_setup(void);         ///< Sets up the pins and clocks
void adc_poll(void);          ///< Starts a sequence of conversions.

void adc_get_dma_results(uint16_t results[2]);

// Debugging
void adc_debug(void);   // Poll ADC and show results

/*! @} */
#endif  /* __SUPERCONDUCTOR_ADC_H__ */
