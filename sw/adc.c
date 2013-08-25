/*
Copyright 2013 Cosmogia
*/

#include <stdio.h>
#include <string.h>

// External dependencies
#include <libopencm3/stm32/f4/adc.h>
#include <libopencm3/stm32/f4/nvic.h>
#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/f4/dma.h>
#include <libopencm3/stm32/f4/gpio.h>

// Drivers
#include "./util.h"
#include "./timer.h"   // for _delay_ms() macro
#include "./adc.h"


// Configuration variables
static bool GLOBAL_ADC_DEBUG_FLAG = false;
static uint16_t GLOBAL_ACD_DMA_BUFFER[16];

// This is a structure of configuration settings for the DMA
typedef struct {
  uint32_t dma;
  uint8_t stream;
  uint32_t channel;
  volatile uint32_t *source;
  uint16_t *destination;
  uint8_t number_of_data;
} dma_config_t;

// This is a structure of configuration settings for the ADC
typedef struct {
  uint32_t adc;
  uint8_t channels[8];
  uint8_t num_channels;
  dma_config_t D;         // <--- notice it includes DMA configuration
} adc_configuration_t;


// ------------------------------------------------------------------ Static
// Enable Vbat (should be in libopemcm3 library)
static void adc_enable_vbat_sensor(void) {
  ADC_CCR |= ADC_CCR_VBATE;
}

static void dma_init(dma_config_t d) {
  // Straight from the manual... The following sequence should be followed to
  // configure a DMA stream x (where x is the stream number):
  // 1. If the stream is enabled, disable it by resetting the EN bit in the
  //    DMA_SxCR register, then read this bit in order to confirm that there is
  //    no ongoing stream operation. Writing  this bit to 0 is not immediately
  //    effective since it is actually written to 0 once all the current
  //    transfers have finished. When the EN bit is read as 0, this means that
  //    the stream is ready to be configured. It is therefore necessary to wait
  //    for the EN bit to be cleared before starting any stream configuration.
  //    All the stream dedicated bits set in the status register (DMA_LISR and
  //    DMA_HISR) from the previous data block DMA transfer should be cleared
  //    before the stream can be re-enabled.
  dma_stream_reset(d.dma, d.stream);
  dma_disable_stream(d.dma, d.stream);
  // 2. Set the peripheral port register address in the DMA_SxPAR register. The
  //    data will be moved from/ to this address to/ from the peripheral port
  //    after the peripheral event.
  dma_set_peripheral_address(d.dma, d.stream, (uint32_t)d.source);
  // 3. Set the memory address in the DMA_SxMA0R register (and in the DMA_SxMA1R
  //    register in the case of a double buffer mode). The data will be written
  //    to or read from
  //  this memory after the peripheral event.
  dma_set_memory_address(d.dma, d.stream, (uint32_t)d.destination);
  // 4. Configure the total number of data items to be transferred in the
  //    DMA_SxNDTR register. After each peripheral event or each beat of the
  //    burst, this value is
  //  decremented.
  dma_set_number_of_data(d.dma, d.stream, d.number_of_data);
  // 5. Select the DMA channel (request) using CHSEL[2:0] in the DMA_SxCR
  //    register.
  dma_channel_select(d.dma, d.stream, d.channel);
  // 6. If the peripheral is intended to be the flow controller and if it
  //    supports this feature, set the PFCTRL bit in the DMA_SxCR register.
  // Nope
  // 7. Configure the stream priority using the PL[1:0] bits in the DMA_SxCR
  //    register.
  dma_set_priority(d.dma, d.stream, DMA_SxCR_PL_MEDIUM);
  // 8. Configure the FIFO usage (enable or disable, threshold in transmission
  //    and reception) Leave as default
  // 9. Configure the:
  //    - data transfer direction,
  dma_set_transfer_mode(d.dma, d.stream, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
  //    - peripheral and memory incremented/fixed mode,
  dma_disable_peripheral_increment_mode(d.dma, d.stream);
  dma_enable_memory_increment_mode(d.dma, d.stream);
  //    - single or burst transactions,
  // Single is default
  //    - peripheral and memory data widths,
  dma_set_memory_size(d.dma, d.stream, DMA_SxCR_MSIZE_16BIT);
  dma_set_peripheral_size(d.dma, d.stream, DMA_SxCR_PSIZE_16BIT);
  //    - Circular mode,
  dma_enable_circular_mode(d.dma, d.stream);
  //    - Double buffer mode
  // Not enabled
  //    - interrupts after half and/or full transfer,
  // Not yet...
  //    - and/or errors in the DMA_SxCR register.
  // Not yet...
  // 10. Activate the stream by setting the EN bit in the DMA_SxCR register.
  dma_enable_stream(d.dma, d.stream);
}

// This function finishes configuring the ADC according to the given
// configuration struct
static void adc_dynamic_init(adc_configuration_t c) {
  // The start of the configuration happens in boardsupport
  // 4. Setup channels
  adc_set_regular_sequence(c.adc, c.num_channels, c.channels);
  // 5. Set single/continouos/discontinouos mode
  adc_set_single_conversion_mode(c.adc);
  // adc_set_continuous_conversion_mode(c.adc);
  adc_enable_discontinuous_mode_regular(c.adc, c.num_channels);
  // 6. Enable/Disable analog watchdog
  adc_disable_analog_watchdog_regular(c.adc);
  adc_disable_analog_watchdog_injected(c.adc);
  // 7. Enable/disable scan mode
  adc_enable_scan_mode(c.adc);
  // 8. Set the alignment of data
  adc_set_right_aligned(c.adc);
  // 9. Set the sample time
  adc_set_sample_time_on_all_channels(c.adc, ADC_SMPR1_SMP_1DOT5CYC);
  // 10. Fast conversion mode?
  // N/A
  // 11. Configure DMA
  adc_set_dma_continue(c.adc);
  adc_enable_dma(c.adc);
  // 12. Set Multi-ADC mode
  adc_set_multi_mode(ADC_CCR_MULTI_INDEPENDENT);
  // 13. Enable the temperature sensor / battery if using
  adc_enable_temperature_sensor();
  adc_enable_vbat_sensor();
  // 14. Turn it on
  adc_power_on(c.adc);
  // 15. Connect it to dma (Note: DMA clocks must be seperately turned on)
  dma_init(c.D);
}

/// Initialization for adc
static void adc_init(void) {
  // This function sets up the ADC1 and ADC3 configuration options.
  // There's several "magic numbers" used in the configuration, so it's
  // pretty fragile.  Don't change anything here unless you have to.
  adc_configuration_t test_adc1 = {
    ADC1,           // Name of ADC peripheral
    {ADC_CHANNEL8, ADC_CHANNEL9},   // Listing of Channels
    1,                                              // Number of channels
    {
      DMA2,                         // Assocaited DMA
      DMA_STREAM0,                  // Assocated DMA Stream
      DMA_SxCR_CHSEL_0,             // DMA Channel Select
      (volatile uint32_t *) &ADC1_DR,                     // Pointer to ADC Data Register
      (uint16_t *) &GLOBAL_ACD_DMA_BUFFER[0],        // Pointer to the DMA memory target
      6                             //
    }
  };

  adc_dynamic_init(test_adc1);

}

// ------------------------------------------------------------------- Public
// TODO(Mike2): Make this dependent on the uint32_t ADC channel
void adc_setup(void) {
  // The goal of this sequence is to have the adc running in regular mode,
  // polling channels continouosly and transferring the results to SRAM
  // via DMA.

  // 1. Turn off ADC for configuration
  adc_off(ADC1);

  // 2. Set the pin configuration
  //    Note that we're setting all these pins to be analog pins, no pull up or
  //    pull down
  rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPBEN);
  gpio_mode_setup(GPIOB, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO0 | GPIO1);    // B0 and B1

  // 3. Setup the clocks
  rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_ADC1EN);
  adc_set_clk_prescale(ADC_CCR_ADCPRE_BY2);
  rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_DMA2EN);


  // The rest of the configuration is handled in adc_init()
  adc_init();
}


void adc_poll() {
  adc_start_conversion_regular(ADC1);
}


void adc_get_dma_results(uint16_t results[2]) {
  results[0] = GLOBAL_ACD_DMA_BUFFER[0];
  results[1] = GLOBAL_ACD_DMA_BUFFER[1];
}

void adc_debug(void) {
  if (!GLOBAL_ADC_DEBUG_FLAG) return;
  adc_poll();
  _delay_ms(20);
  // eprintf("POT: %4i  Temp: %4i  Vref: %4i Vbat: %4i \n"
  //        ,GLOBAL_ACD_DMA_BUFFER[0]
  //        ,GLOBAL_ACD_DMA_BUFFER[1]
  //        ,GLOBAL_ACD_DMA_BUFFER[2]
  //        ,GLOBAL_ACD_DMA_BUFFER[3]);
  eprintf("MAG_X_I: %4i |  MAG_Y_I: %4i |  MAG_Z_I: %4i |  Blank: %4i |  Temperature: %4i |  Vbat: %4i |  COND_I: %4i |  SENS_I: %4i |  VCC_5_I: %4i |  BAT_IRID_V_DIV: %4i |  BAT_CPU_V_DIV: %4i |  VCC_8_DIV: %4i |  ACS_I: %4i |\n",
         GLOBAL_ACD_DMA_BUFFER[0],
         GLOBAL_ACD_DMA_BUFFER[1],
         GLOBAL_ACD_DMA_BUFFER[2],
         GLOBAL_ACD_DMA_BUFFER[3],
         GLOBAL_ACD_DMA_BUFFER[4],
         GLOBAL_ACD_DMA_BUFFER[5],
         GLOBAL_ACD_DMA_BUFFER[8],
         GLOBAL_ACD_DMA_BUFFER[9],
         GLOBAL_ACD_DMA_BUFFER[10],
         GLOBAL_ACD_DMA_BUFFER[11],
         GLOBAL_ACD_DMA_BUFFER[12],
         GLOBAL_ACD_DMA_BUFFER[13],
         GLOBAL_ACD_DMA_BUFFER[14]);
  for (int i = 0; i < 15; i++) {
    GLOBAL_ACD_DMA_BUFFER[i] = 0;
  }
}


/*
int adc_commandler(uint8_t len, const uint8_t data[],
                   uint8_t *replen, uint8_t reply[]) {
  if (len < 2) return NACK_PAYLOAD_SHORT;
  if (len > 2) return NACK_PAYLOAD_LONG;

  check_reply_space(*replen, 4);
  switch (data[0]) {
    case MAG_X_I:
      respond(MAG_X_I, GLOBAL_ACD_DMA_BUFFER[MAG_X_I], replen, reply); break;
    case MAG_Y_I:
      respond(MAG_Y_I, GLOBAL_ACD_DMA_BUFFER[MAG_Y_I], replen, reply); break;
    case MAG_Z_I:
      respond(MAG_Z_I, GLOBAL_ACD_DMA_BUFFER[MAG_Z_I], replen, reply); break;
    case Blank:
      respond(Blank, GLOBAL_ACD_DMA_BUFFER[Blank], replen, reply); break;
    case Temperature:
      respond(Temperature, GLOBAL_ACD_DMA_BUFFER[Temperature], replen, reply); break;
    case Vbat:
      respond(Vbat, GLOBAL_ACD_DMA_BUFFER[Vbat], replen, reply); break;
    case COND_I:
      respond(COND_I, GLOBAL_ACD_DMA_BUFFER[COND_I], replen, reply); break;
    case SENS_I:
      respond(SENS_I, GLOBAL_ACD_DMA_BUFFER[SENS_I], replen, reply); break;
    case VCC_5_I:
      respond(VCC_5_I, GLOBAL_ACD_DMA_BUFFER[VCC_5_I], replen, reply); break;
    case BAT_IRID_V_DIV:
      respond(BAT_IRID_V_DIV, GLOBAL_ACD_DMA_BUFFER[BAT_IRID_V_DIV], replen, reply); break;
    case BAT_CPU_V_DIV:
      respond(BAT_CPU_V_DIV, GLOBAL_ACD_DMA_BUFFER[BAT_CPU_V_DIV], replen, reply); break;
    case VCC_8_DIV:
      respond(VCC_8_DIV, GLOBAL_ACD_DMA_BUFFER[VCC_8_DIV], replen, reply); break;
    case ACS_I:
      respond(ACS_I, GLOBAL_ACD_DMA_BUFFER[ACS_I], replen, reply); break;
    default:
      *replen = 0;
      return NACK_UNKNOWN_COMMAND;
  }
  return 0;
}
*/

// ------------------------------------------------------------ Debug
#ifdef DEBUG
void rcc_show_registers(void);
void adc_show_registers(void);
void adc_show_registers_adc2(void);
void dma_check_overflow(void);
void dma_show_registers(void);

void rcc_show_registers(void) {
  eprintf("\r\n-------------------- RCC REGISTERS -------------------------\n");
  eprintf("RCC_CR = %lx  --- ", RCC_CR);
  print32bits(RCC_CR);
  eprintf("\n");
  eprintf("RCC_PLLCFGR = %lx  --- ", RCC_PLLCFGR);
  print32bits(RCC_PLLCFGR);
  eprintf("\n");
  eprintf("RCC_CFGR = %lx  --- ", RCC_CFGR);
  print32bits(RCC_CFGR);
  eprintf("\n");
  eprintf("RCC_CIR = %lx  --- ", RCC_CIR);
  print32bits(RCC_CIR);
  eprintf("\n");
  eprintf("RCC_AHB1RSTR = %lx  --- ", RCC_AHB1RSTR);
  print32bits(RCC_AHB1RSTR);
  eprintf("\n");
  eprintf("RCC_AHB2RSTR = %lx  --- ", RCC_AHB2RSTR);
  print32bits(RCC_AHB2RSTR);
  eprintf("\n");
  eprintf("RCC_AHB3RSTR = %lx  --- ", RCC_AHB3RSTR);
  print32bits(RCC_AHB3RSTR);
  eprintf("\n");
  /* RCC_BASE + 0x1c Reserved */
  eprintf("RCC_APB1RSTR = %lx  --- ", RCC_APB1RSTR);
  print32bits(RCC_APB1RSTR);
  eprintf("\n");
  eprintf("RCC_APB2RSTR = %lx  --- ", RCC_APB2RSTR);
  print32bits(RCC_APB2RSTR);
  eprintf("\n");
  /* RCC_BASE + 0x28 Reserved */
  /* RCC_BASE + 0x2c Reserved */
  eprintf("RCC_AHB1ENR = %lx  --- ", RCC_AHB1ENR);
  print32bits(RCC_AHB1ENR);
  eprintf("\n");
  eprintf("RCC_AHB2ENR = %lx  --- ", RCC_AHB2ENR);
  print32bits(RCC_AHB2ENR);
  eprintf("\n");
  eprintf("RCC_AHB3ENR = %lx  --- ", RCC_AHB3ENR);
  print32bits(RCC_AHB3ENR);
  eprintf("\n");
  /* RCC_BASE + 0x3c Reserved */
  eprintf("RCC_APB1ENR = %lx  --- ", RCC_APB1ENR);
  print32bits(RCC_APB1ENR);
  eprintf("\n");
  eprintf("RCC_APB2ENR = %lx  --- ", RCC_APB2ENR);
  print32bits(RCC_APB2ENR);
  eprintf("\n");
  /* RCC_BASE + 0x48 Reserved */
  /* RCC_BASE + 0x4c Reserved */
  eprintf("RCC_AHB1LPENR = %lx  --- ", RCC_AHB1LPENR);
  print32bits(RCC_AHB1LPENR);
  eprintf("\n");
  eprintf("RCC_AHB2LPENR = %lx  --- ", RCC_AHB2LPENR);
  print32bits(RCC_AHB2LPENR);
  eprintf("\n");
  eprintf("RCC_AHB3LPENR = %lx  --- ", RCC_AHB3LPENR);
  print32bits(RCC_AHB3LPENR);
  eprintf("\n");
  /* RCC_BASE + 0x5c Reserved */
  eprintf("RCC_APB1LPENR = %lx  --- ", RCC_APB1LPENR);
  print32bits(RCC_APB1LPENR);
  eprintf("\n");
  eprintf("RCC_APB2LPENR = %lx  --- ", RCC_APB2LPENR);
  print32bits(RCC_APB2LPENR);
  eprintf("\n");
  /* RCC_BASE + 0x68 Reserved */
  /* RCC_BASE + 0x6c Reserved */
  eprintf("RCC_BDCR = %lx  --- ", RCC_BDCR);
  print32bits(RCC_BDCR);
  eprintf("\n");
  eprintf("RCC_CSR = %lx  --- ", RCC_CSR);
  print32bits(RCC_CSR);
  eprintf("\n");
  /* RCC_BASE + 0x78 Reserved */
  /* RCC_BASE + 0x7c Reserved */
  eprintf("RCC_SSCGR = %lx  --- ", RCC_SSCGR);
  print32bits(RCC_SSCGR);
  eprintf("\n");
  eprintf("RCC_PLLI2SCFGR = %lx  --- ", RCC_PLLI2SCFGR);
  print32bits(RCC_PLLI2SCFGR);
  eprintf("\n");
}


static void adc_show_common_registers(void) {
  eprintf("----------------------------------------  Common Registers\n");
  /* ADC common (shared) registers */
  eprintf("ADC_CSR = %lx  --- ", ADC_CSR);
  print32bits(ADC_CSR);
  eprintf("\n");
  eprintf("ADC_CCR = %lx  --- ", ADC_CCR);
  print32bits(ADC_CCR);
  eprintf("\n");
  eprintf("ADC_CDR = %lx  --- ", ADC_CDR);
  print32bits(ADC_CDR);
  eprintf("\n");
}

static void adc_show_registers_adc1(void) {
  eprintf("----------------------------------------  ADC1\n");
  eprintf("ADC1_SR = %lx  --- ", ADC1_SR);
  print32bits(ADC1_SR);
  eprintf("\n");
  eprintf("ADC1_CR1 = %lx  --- ", ADC1_CR1);
  print32bits(ADC1_CR1);
  eprintf("\n");
  eprintf("ADC1_CR2 = %lx  --- ", ADC1_CR2);
  print32bits(ADC1_CR2);
  eprintf("\n");
  eprintf("ADC1_SMPR1 = %lx  --- ", ADC1_SMPR1);
  print32bits(ADC1_SMPR1);
  eprintf("\n");
  eprintf("ADC1_SMPR2 = %lx  --- ", ADC1_SMPR2);
  print32bits(ADC1_SMPR2);
  eprintf("\n");
  eprintf("ADC1_JOFR1 = %lx  --- ", ADC1_JOFR1);
  print32bits(ADC1_JOFR1);
  eprintf("\n");
  eprintf("ADC1_JOFR2 = %lx  --- ", ADC1_JOFR2);
  print32bits(ADC1_JOFR2);
  eprintf("\n");
  eprintf("ADC1_JOFR3 = %lx  --- ", ADC1_JOFR3);
  print32bits(ADC1_JOFR3);
  eprintf("\n");
  eprintf("ADC1_JOFR4 = %lx  --- ", ADC1_JOFR4);
  print32bits(ADC1_JOFR4);
  eprintf("\n");
  eprintf("ADC1_HTR = %lx  --- ", ADC1_HTR);
  print32bits(ADC1_HTR);
  eprintf("\n");
  eprintf("ADC1_LTR = %lx  --- ", ADC1_LTR);
  print32bits(ADC1_LTR);
  eprintf("\n");
  eprintf("ADC1_SQR1 = %lx  --- ", ADC1_SQR1);
  print32bits(ADC1_SQR1);
  eprintf("\n");
  eprintf("ADC1_SQR2 = %lx  --- ", ADC1_SQR2);
  print32bits(ADC1_SQR2);
  eprintf("\n");
  eprintf("ADC1_SQR3 = %lx  --- ", ADC1_SQR3);
  print32bits(ADC1_SQR3);
  eprintf("\n");
  eprintf("ADC1_JSQR = %lx  --- ", ADC1_JSQR);
  print32bits(ADC1_JSQR);
  eprintf("\n");
  eprintf("ADC1_JDR1 = %lx  --- ", ADC1_JDR1);
  print32bits(ADC1_JDR1);
  eprintf("\n");
  eprintf("ADC1_JDR2 = %lx  --- ", ADC1_JDR2);
  print32bits(ADC1_JDR2);
  eprintf("\n");
  eprintf("ADC1_JDR3 = %lx  --- ", ADC1_JDR3);
  print32bits(ADC1_JDR3);
  eprintf("\n");
  eprintf("ADC1_JDR4 = %lx  --- ", ADC1_JDR4);
  print32bits(ADC1_JDR4);
  eprintf("\n");
  eprintf("ADC1_DR = %lx  --- ", ADC1_DR);
  print32bits(ADC1_DR);
  eprintf("\n");
}


void adc_show_registers_adc2(void) {
  eprintf("----------------------------------------  ADC2\n");
  eprintf("ADC2_SR = %lx  --- ", ADC2_SR);
  print32bits(ADC2_SR);
  eprintf("\n");
  eprintf("ADC2_CR1 = %lx  --- ", ADC2_CR1);
  print32bits(ADC2_CR1);
  eprintf("\n");
  eprintf("ADC2_CR2 = %lx  --- ", ADC2_CR2);
  print32bits(ADC2_CR2);
  eprintf("\n");
  eprintf("ADC2_SMPR1 = %lx  --- ", ADC2_SMPR1);
  print32bits(ADC2_SMPR1);
  eprintf("\n");
  eprintf("ADC2_SMPR2 = %lx  --- ", ADC2_SMPR2);
  print32bits(ADC2_SMPR2);
  eprintf("\n");
  eprintf("ADC2_JOFR1 = %lx  --- ", ADC2_JOFR1);
  print32bits(ADC2_JOFR1);
  eprintf("\n");
  eprintf("ADC2_JOFR2 = %lx  --- ", ADC2_JOFR2);
  print32bits(ADC2_JOFR2);
  eprintf("\n");
  eprintf("ADC2_JOFR3 = %lx  --- ", ADC2_JOFR3);
  print32bits(ADC2_JOFR3);
  eprintf("\n");
  eprintf("ADC2_JOFR4 = %lx  --- ", ADC2_JOFR4);
  print32bits(ADC2_JOFR4);
  eprintf("\n");
  eprintf("ADC2_HTR = %lx  --- ", ADC2_HTR);
  print32bits(ADC2_HTR);
  eprintf("\n");
  eprintf("ADC2_LTR = %lx  --- ", ADC2_LTR);
  print32bits(ADC2_LTR);
  eprintf("\n");
  eprintf("ADC2_SQR1 = %lx  --- ", ADC2_SQR1);
  print32bits(ADC2_SQR1);
  eprintf("\n");
  eprintf("ADC2_SQR2 = %lx  --- ", ADC2_SQR2);
  print32bits(ADC2_SQR2);
  eprintf("\n");
  eprintf("ADC2_SQR3 = %lx  --- ", ADC2_SQR3);
  print32bits(ADC2_SQR3);
  eprintf("\n");
  eprintf("ADC2_JSQR = %lx  --- ", ADC2_JSQR);
  print32bits(ADC2_JSQR);
  eprintf("\n");
  eprintf("ADC2_JDR1 = %lx  --- ", ADC2_JDR1);
  print32bits(ADC2_JDR1);
  eprintf("\n");
  eprintf("ADC2_JDR2 = %lx  --- ", ADC2_JDR2);
  print32bits(ADC2_JDR2);
  eprintf("\n");
  eprintf("ADC2_JDR3 = %lx  --- ", ADC2_JDR3);
  print32bits(ADC2_JDR3);
  eprintf("\n");
  eprintf("ADC2_JDR4 = %lx  --- ", ADC2_JDR4);
  print32bits(ADC2_JDR4);
  eprintf("\n");
  eprintf("ADC2_DR = %lx  --- ", ADC2_DR);
  print32bits(ADC2_DR);
  eprintf("\n");
}

static void adc_show_registers_adc3(void) {
  eprintf("----------------------------------------  ADC3\n");
  eprintf("ADC3_SR = %lx  --- ", ADC3_SR);
  print32bits(ADC3_SR);
  eprintf("\n");
  eprintf("ADC3_CR1 = %lx  --- ", ADC3_CR1);
  print32bits(ADC3_CR1);
  eprintf("\n");
  eprintf("ADC3_CR2 = %lx  --- ", ADC3_CR2);
  print32bits(ADC3_CR2);
  eprintf("\n");
  eprintf("ADC3_SMPR1 = %lx  --- ", ADC3_SMPR1);
  print32bits(ADC3_SMPR1);
  eprintf("\n");
  eprintf("ADC3_SMPR2 = %lx  --- ", ADC3_SMPR2);
  print32bits(ADC3_SMPR2);
  eprintf("\n");
  eprintf("ADC3_JOFR1 = %lx  --- ", ADC3_JOFR1);
  print32bits(ADC3_JOFR1);
  eprintf("\n");
  eprintf("ADC3_JOFR2 = %lx  --- ", ADC3_JOFR2);
  print32bits(ADC3_JOFR2);
  eprintf("\n");
  eprintf("ADC3_JOFR3 = %lx  --- ", ADC3_JOFR3);
  print32bits(ADC3_JOFR3);
  eprintf("\n");
  eprintf("ADC3_JOFR4 = %lx  --- ", ADC3_JOFR4);
  print32bits(ADC3_JOFR4);
  eprintf("\n");
  eprintf("ADC3_HTR = %lx  --- ", ADC3_HTR);
  print32bits(ADC3_HTR);
  eprintf("\n");
  eprintf("ADC3_LTR = %lx  --- ", ADC3_LTR);
  print32bits(ADC3_LTR);
  eprintf("\n");
  eprintf("ADC3_SQR1 = %lx  --- ", ADC3_SQR1);
  print32bits(ADC3_SQR1);
  eprintf("\n");
  eprintf("ADC3_SQR2 = %lx  --- ", ADC3_SQR2);
  print32bits(ADC3_SQR2);
  eprintf("\n");
  eprintf("ADC3_SQR3 = %lx  --- ", ADC3_SQR3);
  print32bits(ADC3_SQR3);
  eprintf("\n");
  eprintf("ADC3_JSQR = %lx  --- ", ADC3_JSQR);
  print32bits(ADC3_JSQR);
  eprintf("\n");
  eprintf("ADC3_JDR1 = %lx  --- ", ADC3_JDR1);
  print32bits(ADC3_JDR1);
  eprintf("\n");
  eprintf("ADC3_JDR2 = %lx  --- ", ADC3_JDR2);
  print32bits(ADC3_JDR2);
  eprintf("\n");
  eprintf("ADC3_JDR3 = %lx  --- ", ADC3_JDR3);
  print32bits(ADC3_JDR3);
  eprintf("\n");
  eprintf("ADC3_JDR4 = %lx  --- ", ADC3_JDR4);
  print32bits(ADC3_JDR4);
  eprintf("\n");
  eprintf("ADC3_DR = %lx  --- ", ADC3_DR);
  print32bits(ADC3_DR);
  eprintf("\n");
}


void adc_show_registers(void) {
  eprintf("\r\n-------------------- ADC REGISTERS -------------------------\n");
  adc_show_common_registers();
  adc_show_registers_adc1();
  // adc_show_registers_adc2();
  adc_show_registers_adc3();
}


void dma_check_overflow(void) {
  // TODO(MA)
}


static void dma_show_registers_dma2(void) {
  eprintf("--------------------------------------------------DMA 2\n");
  eprintf("DMA2_LISR = %lx\n", DMA2_LISR);
  eprintf("DMA2_HISR = %lx\n", DMA2_HISR);
  eprintf("DMA2_LIFCR = %lx\n", DMA2_LIFCR);
  eprintf("DMA2_HIFCR = %lx\n", DMA2_HIFCR);
  eprintf("DMA2_S0CR = %lx\n", DMA2_S0CR);
  eprintf("DMA2_S1CR = %lx\n", DMA2_S1CR);
  eprintf("DMA2_S2CR = %lx\n", DMA2_S2CR);
  eprintf("DMA2_S3CR = %lx\n", DMA2_S3CR);
  eprintf("DMA2_S4CR = %lx\n", DMA2_S4CR);
  eprintf("DMA2_S5CR = %lx\n", DMA2_S5CR);
  eprintf("DMA2_S6CR = %lx\n", DMA2_S6CR);
  eprintf("DMA2_S7CR = %lx\n", DMA2_S7CR);
  eprintf("DMA2_S0NDTR = %lx\n", DMA2_S0NDTR);
  eprintf("DMA2_S1NDTR = %lx\n", DMA2_S1NDTR);
  eprintf("DMA2_S2NDTR = %lx\n", DMA2_S2NDTR);
  eprintf("DMA2_S3NDTR = %lx\n", DMA2_S3NDTR);
  eprintf("DMA2_S4NDTR = %lx\n", DMA2_S4NDTR);
  eprintf("DMA2_S5NDTR = %lx\n", DMA2_S5NDTR);
  eprintf("DMA2_S6NDTR = %lx\n", DMA2_S6NDTR);
  eprintf("DMA2_S7NDTR = %lx\n", DMA2_S7NDTR);
  eprintf("DMA2_S0PAR = %lx\n", (uint32_t)DMA2_S0PAR);
  eprintf("DMA2_S1PAR = %lx\n", (uint32_t)DMA2_S1PAR);
  eprintf("DMA2_S2PAR = %lx\n", (uint32_t)DMA2_S2PAR);
  eprintf("DMA2_S3PAR = %lx\n", (uint32_t)DMA2_S3PAR);
  eprintf("DMA2_S4PAR = %lx\n", (uint32_t)DMA2_S4PAR);
  eprintf("DMA2_S5PAR = %lx\n", (uint32_t)DMA2_S5PAR);
  eprintf("DMA2_S6PAR = %lx\n", (uint32_t)DMA2_S6PAR);
  eprintf("DMA2_S7PAR = %lx\n", (uint32_t)DMA2_S7PAR);
  eprintf("DMA2_S0M0AR = %lx\n", (uint32_t)DMA2_S0M0AR);
  eprintf("DMA2_S1M0AR = %lx\n", (uint32_t)DMA2_S1M0AR);
  eprintf("DMA2_S2M0AR = %lx\n", (uint32_t)DMA2_S2M0AR);
  eprintf("DMA2_S3M0AR = %lx\n", (uint32_t)DMA2_S3M0AR);
  eprintf("DMA2_S4M0AR = %lx\n", (uint32_t)DMA2_S4M0AR);
  eprintf("DMA2_S5M0AR = %lx\n", (uint32_t)DMA2_S5M0AR);
  eprintf("DMA2_S6M0AR = %lx\n", (uint32_t)DMA2_S6M0AR);
  eprintf("DMA2_S7M0AR = %lx\n", (uint32_t)DMA2_S7M0AR);
  eprintf("DMA2_S0M1AR = %lx\n", (uint32_t)DMA2_S0M1AR);
  eprintf("DMA2_S1M1AR = %lx\n", (uint32_t)DMA2_S1M1AR);
  eprintf("DMA2_S2M1AR = %lx\n", (uint32_t)DMA2_S2M1AR);
  eprintf("DMA2_S3M1AR = %lx\n", (uint32_t)DMA2_S3M1AR);
  eprintf("DMA2_S4M1AR = %lx\n", (uint32_t)DMA2_S4M1AR);
  eprintf("DMA2_S5M1AR = %lx\n", (uint32_t)DMA2_S5M1AR);
  eprintf("DMA2_S6M1AR = %lx\n", (uint32_t)DMA2_S6M1AR);
  eprintf("DMA2_S7M1AR = %lx\n", (uint32_t)DMA2_S7M1AR);
  eprintf("DMA2_S0FCR = %lx\n", DMA2_S0FCR);
  eprintf("DMA2_S1FCR = %lx\n", DMA2_S1FCR);
  eprintf("DMA2_S2FCR = %lx\n", DMA2_S2FCR);
  eprintf("DMA2_S3FCR = %lx\n", DMA2_S3FCR);
  eprintf("DMA2_S4FCR = %lx\n", DMA2_S4FCR);
  eprintf("DMA2_S5FCR = %lx\n", DMA2_S5FCR);
  eprintf("DMA2_S6FCR = %lx\n", DMA2_S6FCR);
  eprintf("DMA2_S7FCR = %lx\n", DMA2_S7FCR);
}

static void dma_show_registers_dma1(void) {
  eprintf("--------------------------------------------------DMA 1\n");
  eprintf("DMA low interrupt status register (DMA1_LISR)   = %lx --- ", DMA1_LISR);
  print32bits(DMA1_LISR);
  eprintf("\n");
  eprintf("DMA high interrupt status register (DMA1_HISR)    = %lx --- ", DMA1_HISR);
  print32bits(DMA1_HISR);
  eprintf("\n");
  eprintf("DMA low interrupt flag clear register (DMA1_LIFCR)  = %lx --- ", DMA1_LIFCR);
  print32bits(DMA1_LIFCR);
  eprintf("\n");
  eprintf("DMA high interrupt flag clear register (DMA1_HIFCR) = %lx --- ", DMA1_HIFCR);
  print32bits(DMA1_HIFCR);
  eprintf("\n");
  eprintf("DMA Stream 0 configuration register (DMA1_S0CR)   = %lx --- ", DMA1_S0CR);
  print32bits(DMA1_S0CR);
  eprintf("\n");
  eprintf("DMA Stream 1 configuration register (DMA1_S1CR)   = %lx --- ", DMA1_S1CR);
  print32bits(DMA1_S1CR);
  eprintf("\n");
  eprintf("DMA Stream 2 configuration register (DMA1_S2CR)   = %lx --- ", DMA1_S2CR);
  print32bits(DMA1_S2CR);
  eprintf("\n");
  eprintf("DMA Stream 3 configuration register (DMA1_S3CR)   = %lx --- ", DMA1_S3CR);
  print32bits(DMA1_S3CR);
  eprintf("\n");
  eprintf("DMA Stream 4 configuration register (DMA1_S4CR)   = %lx --- ", DMA1_S4CR);
  print32bits(DMA1_S4CR);
  eprintf("\n");
  eprintf("DMA Stream 5 configuration register (DMA1_S5CR)   = %lx --- ", DMA1_S5CR);
  print32bits(DMA1_S5CR);
  eprintf("\n");
  eprintf("DMA Stream 6 configuration register (DMA1_S6CR)   = %lx --- ", DMA1_S6CR);
  print32bits(DMA1_S6CR);
  eprintf("\n");
  eprintf("DMA Stream 7 configuration register (DMA1_S7CR)   = %lx --- ", DMA1_S7CR);
  print32bits(DMA1_S7CR);
  eprintf("\n");
  /* DMA Stream x number of data register (DMA_SxNDTR) */
  eprintf("DMA1_S0NDTR = %lx\n", DMA1_S0NDTR);
  eprintf("DMA1_S1NDTR = %lx\n", DMA1_S1NDTR);
  eprintf("DMA1_S2NDTR = %lx\n", DMA1_S2NDTR);
  eprintf("DMA1_S3NDTR = %lx\n", DMA1_S3NDTR);
  eprintf("DMA1_S4NDTR = %lx\n", DMA1_S4NDTR);
  eprintf("DMA1_S5NDTR = %lx\n", DMA1_S5NDTR);
  eprintf("DMA1_S6NDTR = %lx\n", DMA1_S6NDTR);
  eprintf("DMA1_S7NDTR = %lx\n", DMA1_S7NDTR);
  /* DMA Stream x peripheral address register (DMA_SxPAR) */
  eprintf("DMA1_S0PAR = %lx\n", (uint32_t)DMA1_S0PAR);
  eprintf("DMA1_S1PAR = %lx\n", (uint32_t)DMA1_S1PAR);
  eprintf("DMA1_S2PAR = %lx\n", (uint32_t)DMA1_S2PAR);
  eprintf("DMA1_S3PAR = %lx\n", (uint32_t)DMA1_S3PAR);
  eprintf("DMA1_S4PAR = %lx\n", (uint32_t)DMA1_S4PAR);
  eprintf("DMA1_S5PAR = %lx\n", (uint32_t)DMA1_S5PAR);
  eprintf("DMA1_S6PAR = %lx\n", (uint32_t)DMA1_S6PAR);
  eprintf("DMA1_S7PAR = %lx\n", (uint32_t)DMA1_S7PAR);
  /* DMA Stream x memory address 0 register (DMA_SxM0AR) */
  eprintf("DMA1_S0M0AR = %lx\n", (uint32_t)DMA1_S0M0AR);
  eprintf("DMA1_S1M0AR = %lx\n", (uint32_t)DMA1_S1M0AR);
  eprintf("DMA1_S2M0AR = %lx\n", (uint32_t)DMA1_S2M0AR);
  eprintf("DMA1_S3M0AR = %lx\n", (uint32_t)DMA1_S3M0AR);
  eprintf("DMA1_S4M0AR = %lx\n", (uint32_t)DMA1_S4M0AR);
  eprintf("DMA1_S5M0AR = %lx\n", (uint32_t)DMA1_S5M0AR);
  eprintf("DMA1_S6M0AR = %lx\n", (uint32_t)DMA1_S6M0AR);
  eprintf("DMA1_S7M0AR = %lx\n", (uint32_t)DMA1_S7M0AR);
  /* DMA Stream x memory address 1 register (DMA_SxM1AR) */
  eprintf("DMA1_S0M1AR = %lx\n", (uint32_t)DMA1_S0M1AR);
  eprintf("DMA1_S1M1AR = %lx\n", (uint32_t)DMA1_S1M1AR);
  eprintf("DMA1_S2M1AR = %lx\n", (uint32_t)DMA1_S2M1AR);
  eprintf("DMA1_S3M1AR = %lx\n", (uint32_t)DMA1_S3M1AR);
  eprintf("DMA1_S4M1AR = %lx\n", (uint32_t)DMA1_S4M1AR);
  eprintf("DMA1_S5M1AR = %lx\n", (uint32_t)DMA1_S5M1AR);
  eprintf("DMA1_S6M1AR = %lx\n", (uint32_t)DMA1_S6M1AR);
  eprintf("DMA1_S7M1AR = %lx\n", (uint32_t)DMA1_S7M1AR);
  /* DMA Stream x FIFO control register (DMA_SxFCR) */
  eprintf("DMA1_S0FCR = %lx\n", DMA1_S0FCR);
  eprintf("DMA1_S1FCR = %lx\n", DMA1_S1FCR);
  eprintf("DMA1_S2FCR = %lx\n", DMA1_S2FCR);
  eprintf("DMA1_S3FCR = %lx\n", DMA1_S3FCR);
  eprintf("DMA1_S4FCR = %lx\n", DMA1_S4FCR);
  eprintf("DMA1_S5FCR = %lx\n", DMA1_S5FCR);
  eprintf("DMA1_S6FCR = %lx\n", DMA1_S6FCR);
  eprintf("DMA1_S7FCR = %lx\n", DMA1_S7FCR);
}

// Debug
void dma_show_registers(void) {
  eprintf("\r\n----------------------DMA REGISTERS HERE--------------------\n");
  dma_show_registers_dma1();
  dma_show_registers_dma2();
}
#endif // DEBUG
