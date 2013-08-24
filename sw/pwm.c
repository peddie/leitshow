/*
Copyright 2013 Cosmogia
*/
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>     // for atoi
#include <math.h>       // for round

#include <libopencm3/stm32/f4/timer.h>
#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/f4/nvic.h>
#include <libopencm3/stm32/f4/gpio.h>

#include "./util.h"

#include "./pwm.h"


/// The timer ticks per microsecond.  Can be adjusting by measuring
/// pwm on o-scope
#define PWM_TIMER_TICKS_PER_MICROSECOND 84.0

/// PWM Frequency, default is 62 kHz
#define PWM_FREQUENCY_kHz 62.0

/// PWM Period, set automatically by the options above
#define PWM_PERIOD ((1.0/PWM_FREQUENCY_kHz) * 1000.0 \
                    * PWM_TIMER_TICKS_PER_MICROSECOND / 2)


/*
                                --- FTFM (with edits) ---
Bullet points are the steps needed.

Pulse width modulation mode allows you to generate a signal with a frequency determined
by the value of the TIMx_ARR register (the period) and a duty cycle determined by the value of the
TIMx_CCRx register (the output compare value).
  -- Set TIMx_ARR to desired frequency
  -- Set TIMx_CCRx to desired duty cycle

The PWM mode can be selected independently on each channel (one PWM per OCx
output) by writing 110 (PWM mode 1) or ‘111 (PWM mode 2) in the OCxM bits in the
TIMx_CCMRx register.
  -- Write PWM Mode 1 or PWM Mode 2 to OCxM bits in TIMx_CCMRx register

You must enable the corresponding preload register by setting the
OCxPE bit in the TIMx_CCMRx register, and eventually the auto-reload preload register by
setting the ARPE bit in the TIMx_CR1 register.
  -- Set corresponding OCxPE bit in TIMx_CCMRx register
  -- Set ARPE bit in TIMx_CR1

As the preload registers are transferred to the shadow registers only when an update event
occurs, before starting the counter, you have to initialize all the registers by setting the UG
bit in the TIMx_EGR register.
  -- set UG bit in TIMx_EGR register

OCx polarity is software programmable using the CCxP bit in the TIMx_CCER register. It
can be programmed as active high or active low. OCx output is enabled by the CCxE bit in
the TIMx_CCER register. Refer to the TIMx_CCERx register description for more details.
  -- set desired polarity in TIMx_CCER
  -- set CCxE bit in TIMx_CCER  (enable output)
*/
static void
pwm_init(uint32_t timer,
         uint8_t channel,
         uint32_t period) {
  // Convert channel number to internal rep
  enum tim_oc_id chan;
  switch (channel) {
    case 1:   chan = TIM_OC1; break;
    case 2:   chan = TIM_OC2; break;
    case 3:   chan = TIM_OC3; break;
    case 4:   chan = TIM_OC4; break;
    default: assert(false); chan = -1; break;
  }

  // Timer Base Configuration
  // timer_reset(timer);
  timer_set_mode(timer, TIM_CR1_CKD_CK_INT, // clock division
                        TIM_CR1_CMS_EDGE,   // Center-aligned mode selection
                        TIM_CR1_DIR_UP);    // TIMx_CR1 DIR: Direction
  timer_continuous_mode(timer);             // Disables TIM_CR1_OPM (One pulse mode)
  timer_set_period(timer, period);                    // Sets TIMx_ARR
  timer_set_prescaler(timer, 1);               // Adjusts speed of timer
  timer_set_clock_division(timer, 0);            // Adjusts speed of timer
  timer_set_master_mode(timer, TIM_CR2_MMS_UPDATE);   // Master Mode Selection
  timer_enable_preload(timer);                        // Set ARPE bit in TIMx_CR1

  // Channel-specific settings
  timer_set_oc_value(timer, chan, 0);             // sets TIMx_CCRx
  timer_set_oc_mode(timer, chan, TIM_OCM_PWM1);   // Sets PWM Mode 1
  timer_enable_oc_preload(timer, chan);           // Sets OCxPE in TIMx_CCMRx
  timer_set_oc_polarity_high(timer, chan);        // set desired polarity in TIMx_CCER
  timer_enable_oc_output(timer, chan);             // set CCxE bit in TIMx_CCER  (enable output)

  // Initialize all counters in the register
  switch (timer) {
    case TIM1:  TIM1_EGR |= TIM_EGR_UG; break;
    case TIM2:  TIM2_EGR |= TIM_EGR_UG; break;
    case TIM3:  TIM3_EGR |= TIM_EGR_UG; break;
    case TIM4:  TIM4_EGR |= TIM_EGR_UG; break;
    case TIM5:  TIM5_EGR |= TIM_EGR_UG; break;
    case TIM6:  TIM6_EGR |= TIM_EGR_UG; break;
    case TIM7:  TIM7_EGR |= TIM_EGR_UG; break;
    case TIM8:  TIM8_EGR |= TIM_EGR_UG; break;
    default: assert(false); break;
    }
}

void pwm_setup(void) {
  // Timer 5
  rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM5EN);
  pwm_init(TIM5, 1, PWM_PERIOD);
  pwm_init(TIM5, 2, PWM_PERIOD);
  pwm_init(TIM5, 3, PWM_PERIOD);
  pwm_init(TIM5, 4, PWM_PERIOD);

  // LED channels = PA0..3
  rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPAEN);

  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  GPIO0 | GPIO1 | GPIO2 | GPIO3);
  // AF2 = TIM4_CH1..4
  gpio_set_af(GPIOA, GPIO_AF2, GPIO0 | GPIO1 | GPIO2 | GPIO3);

  timer_enable_counter(TIM5);
}


void pwm_set_duty(uint32_t timer, uint8_t channel, float dc) {
  assert(timer == TIM1 ||
         timer == TIM2 ||
         timer == TIM3 ||
         timer == TIM4 ||
         timer == TIM5 ||
         timer == TIM6 ||
         timer == TIM7 ||
         timer == TIM8);
  assert(channel == TIM_OC1 ||
         channel == TIM_OC2 ||
         channel == TIM_OC3 ||
         channel == TIM_OC4);
  uint32_t value;
  if (dc > 1.0) {
    dc = 1.0;
  } else if (dc < 0.0) {
    dc = 0.0;
  }

  value = lround(dc*PWM_PERIOD);

  //eprintf("Duty Cycle Set to %.2f%% [%lu of %lu] \n", dc*100.0,
  //value, (uint32_t)PWM_PERIOD);
  timer_set_oc_value(timer, channel, value);  // sets TIMx_CCRx
}
