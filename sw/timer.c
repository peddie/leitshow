/*
  \author Michael Aherne (michael.aherne@cosmogia.com)

  \details
  This file sets up both of the 32-bit timers on the STM -- Timer2 and Timer6

  Timer 2 is used for tick() and tock() functions, which measure execution time.

  Timer 1 is used for schedule_callback(), which can schedule functions for
  callback at a later time.

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
*/
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>     // for atol
#include <string.h>

#include <libopencm3/stm32/f4/timer.h>
#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/f4/nvic.h>
#include <libopencm3/stm32/f4/gpio.h>

#include "./util.h"
// Debugging includes
#include "./timer.h"

/// These are conversions for the timers between ticks and microseconds
static const uint32_t TICKS_PER_MICROSECOND = 63;
static const float ASYNC_TCKS_PER_MICROSECOND = 1.0/390.095291137695;  // Experimentally determined

/// This is the calibration of the tick/tock functions
static uint32_t TARE = 0;

// schedule_callback globals
static uint32_t async_timer;
static void (*f_ptr)(void) = NULL;
static bool async_set = false;

/// Useful constants
static const uint32_t MAX_PERIOD_32BIT_TIMER = 0xffffffff;
static const uint32_t MAX_PERIOD_16BIT_TIMER = 0xffff;
static const uint32_t NO_PRESCALER = 0x1;
static const uint32_t NO_DIVIDER = TIM_CR1_CKD_CK_INT;



/** \brief Initialize a timer

    \details This function initializes a basic upcounting timer with the given
    period, prescaler and divider. Overflows generate an interrupt request.
     - The period controls the amount of time before a rollover occurs
     - The prescale and divider both control the speed/resolution of the timer.
       With a high prescaler or divider, each tick of the timer takes longer, so
       the overall timer can measure a longer period, but at the expense of the
       smallest resolution.

    \warning You must use nvic_enable_irq() to enable the relevant Interrupt
             Request.  Otherwise an overflow will cause a jump to a blank space
             in memory and the system will freeze.

*/
static void
generic_timer_init(uint32_t timer,
                   uint32_t period,
                   uint32_t prescale,
                   uint32_t divide) {
  /* Time Base configuration */
  timer_reset(timer);
  timer_set_mode(timer, divide,
                        TIM_CR1_CMS_EDGE,
                        TIM_CR1_DIR_UP);
  timer_set_period(timer, period);
  timer_set_prescaler(timer, prescale);
  timer_set_clock_division(timer, divide);

  // Timers can be slaves of other timers, in order to count REALLY long times.
  // We don't support slavery, so this is set in a default master mode.
  timer_set_master_mode(timer, TIM_CR2_MMS_UPDATE);

  /* Enable interrupts on every overflow */
  timer_enable_irq(timer, TIM_DIER_UIE);

  // Clear the registers
  timer_generate_event(timer, TIM_EGR_UG);

  // Start the timer
  timer_enable_counter(timer);
}

static void enable_timer_clock(uint32_t timer) {
  switch (timer) {
    case TIM1:  rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_TIM1EN);  break;
    case TIM2:  rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM2EN);  break;
    case TIM3:  rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM3EN);  break;
    case TIM4:  rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM4EN);  break;
    case TIM5:  rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM5EN);  break;
    case TIM6:  rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM6EN);  break;
    case TIM7:  rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM7EN);  break;
    case TIM8:  rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_TIM8EN);  break;
    // case TIM9:  rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_TIM9EN);  break;
    // case TIM10: rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_TIM10EN); break;
    // case TIM11: rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_TIM11EN); break;
    // case TIM12: rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM12EN); break;
    // case TIM13: rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM13EN); break;
    // case TIM14: rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM14EN); break;
    default: assert(false); break;
  }
}

static void enable_nvic_timer_irq(uint32_t timer) {
    switch (timer) {
    case TIM1: nvic_enable_irq(NVIC_TIM1_UP_TIM10_IRQ); break; // Timer 1 has multiple irqs, must set explicitly
    case TIM2: nvic_enable_irq(NVIC_TIM2_IRQ); break;
    case TIM3: nvic_enable_irq(NVIC_TIM3_IRQ); break;
    case TIM4: nvic_enable_irq(NVIC_TIM4_IRQ); break;
    case TIM5: nvic_enable_irq(NVIC_TIM5_IRQ); break;
    case TIM6: assert(false); break; // Timer 6 has a special-purpose irq. Must set explicitly
    case TIM7: nvic_enable_irq(NVIC_TIM7_IRQ); break;
    default: assert(false); break;
  }
}

static int32_t tare_check(void) {
  uint32_t t = tick();
  t = tock(t);
  return (int32_t)t;
}

static void tare_adjust(void) {
  // uint32_t tare_previous = TARE;
  uint32_t tare_adjust_value;
  uint8_t loop_counter = 0;
  TARE = 0;
  tare_adjust_value = tare_check();
  while (tare_adjust_value != 0 && loop_counter++ < 30) {
    TARE += tare_adjust_value;
    tare_adjust_value = tare_check();
  // eprintf("TARE adjusted from %lu to %lu \n", tare_previous, TARE);
    }
}

static void timer2_setup(void) {
  // --------------------------- Timer 2
  // Enable the clock for this timer
  rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM2EN);

  // Enable the irq (don't forget to have tim2_isr() defined!)
  nvic_enable_irq(NVIC_TIM2_IRQ);

  // Enable the timer itself.
  // - We used the maximum period to get the most time before a rollover.
  // - We use a prescaler of 1 to get the fastest counter (highest resolution)
  // - We use no divider for the same reason (high resolution)
  generic_timer_init(TIM2, MAX_PERIOD_32BIT_TIMER, NO_PRESCALER, NO_DIVIDER);

  // Calibrate timer2 tare value:
  tare_adjust();
}


static void async_timer_setup(uint32_t timer, uint16_t prescaler) {
  async_timer = timer;
  // Enable the clock
  enable_timer_clock(timer);
  // Enable the irq (don't forget to have the ***_isr() defined!)
  enable_nvic_timer_irq(timer);
  // Enable the timer itself

  timer_reset(timer);
  timer_set_mode(timer, TIM_CR1_CKD_CK_INT,
                        TIM_CR1_CMS_EDGE,
                        TIM_CR1_DIR_DOWN);
  timer_one_shot_mode(timer);
  timer_set_period(timer, MAX_PERIOD_16BIT_TIMER);
  timer_set_prescaler(timer, prescaler);

  // Timers can be slaves of other timers, in order to count REALLY long times.
  // We don't support slavery, so this is set in a default master mode.
  timer_set_master_mode(timer, TIM_CR2_MMS_UPDATE);

  // Clear the registers
  timer_generate_event(timer, TIM_EGR_UG);

  /* Enable interrupts on every overflow */
  timer_enable_irq(timer, TIM_DIER_UIE);
}

void _delay_us(uint32_t uS) {
  uint32_t t = tick();
  uint32_t compare_ticks = uS * TICKS_PER_MICROSECOND;
  do {
    // nothing
  } while (tock(t) < compare_ticks);
}

void _delay_ms(uint32_t ms) {
  uint32_t t = tick();
  uint32_t compare_ticks = ms * TICKS_PER_MICROSECOND * 1000;
  do {
    // nothing
  } while (tock(t) < compare_ticks);
}

// ===================== Public Functions =========================
// -------------------------------------------------------- Startup
void timer_setup(void) {
  // Used for tick/tock.  General uS counter.
  timer2_setup();

  // Used for schedule_callback()
  async_timer_setup(TIM1, 0xffff);
}

// -------------------------------------------------------- Callback
// This sets Timer1 to a certain preloaded counter, which will
// count down to 0.  At 0, it will call a function.
// Boom, asynchronisity.
int8_t schedule_callback(void * v, uint32_t uS) {
  // Make sure we're not asking for more than the maximum period
  float uS_ticks = uS*ASYNC_TCKS_PER_MICROSECOND;
  // eprintf("Got %lu, converted to %f via %f.\n", uS, uS_ticks, ASYNC_TCKS_PER_MICROSECOND);

  // it's very important to lock the system if this is called incorrectly.
  // Otherwise we could leave a burn wire on or do something else really bad.
  if ((uint64_t)uS_ticks > MAX_PERIOD_16BIT_TIMER) return -1;
  if (async_set) return -1;
  assert((uint64_t)uS_ticks <= MAX_PERIOD_16BIT_TIMER);
  assert(!async_set);
  f_ptr = v;
  async_set = true;
  timer_set_counter(async_timer, (uint16_t)(uS_ticks+0.5));
  timer_enable_counter(async_timer);
  return 0;
}

// -------------------------------------------------------- TickTock
uint32_t tick() {
  return timer_get_counter(TIM2);
}

uint32_t tock(uint32_t ticknumber) {
  uint32_t t = timer_get_counter(TIM2);
  return (t - ticknumber) - TARE;
  // if (t > ticknumber)
  //   return (t - ticknumber) - TARE;
  // else
  //   return (MAX_PERIOD_32BIT_TIMER - (ticknumber - t)) - TARE;
}

uint32_t tock_us(uint32_t ticknumber) {
  uint32_t t = tock(ticknumber);
  // This will truncate, which may not be as accurate as rounding, but
  // will happen in constant time, which is important for the tare value.
  return (t/TICKS_PER_MICROSECOND);
}

// -------------------------------------------------------- ISRs
void
tim2_isr(void) {
  timer_clear_flag(TIM2, TIM_SR_UIF);
  // eprintf("ROLLOVER\n");
}

// Callback
void tim1_up_tim10_isr(void)
{
  timer_clear_flag(async_timer, TIM_SR_UIF);
  if (f_ptr != NULL) {
    f_ptr();
    f_ptr = NULL;   // reset the function pointer
  }
  async_set = false;  // allow calls to schedule_callback()
}


