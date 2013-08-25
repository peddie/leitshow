#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <libopencm3/stm32/f4/gpio.h>
#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/f4/timer.h>
#include "./timer.h"
#include "./pwm.h"
#include "./adc.h"
#include "./leitshow.h"


#define PERIOD_US 91

#define N_CHANS 4
#define RED 0
#define GREEN 1
#define BLUE 2
#define WHITE 3

#define PWM_TIMER TIM5
#define PWM_TIM_CHAN_RED TIM_OC1
#define PWM_TIM_CHAN_GREEN TIM_OC2
#define PWM_TIM_CHAN_BLUE TIM_OC3
#define PWM_TIM_CHAN_WHITE TIM_OC4

volatile int foo;

static void setup_peripherals(void) {
  // Setup the relevant clocks
  rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]);
  timer_setup();
  // enable FPU
  unsigned int *cpacr = (unsigned int *) 0xE000ED88;
  *cpacr |= (0xf << 20);

  // output
  pwm_setup();

  // input
  adc_setup();
}

static void set_all_chans(float percent[N_CHANS]) {
  pwm_set_duty(PWM_TIMER,PWM_TIM_CHAN_RED, percent[0]);
  pwm_set_duty(PWM_TIMER,PWM_TIM_CHAN_GREEN, percent[1]);
  pwm_set_duty(PWM_TIMER,PWM_TIM_CHAN_BLUE, percent[2]);
  pwm_set_duty(PWM_TIMER,PWM_TIM_CHAN_WHITE, percent[3]);
}

static void overflow(void) {
  float brt[4]={1,1,1,1};
  set_all_chans(brt);
  while(1);
}

int main(void) {

  setup_peripherals();

  float brt[N_CHANS] = {0};

  uint16_t adc[2]={22,22};

  filter_setup();

  while (1) {
    uint32_t ts= tick();
    adc_get_dma_results(adc);
    adc_poll();

    //    leitshow_step(adc[0], brt);
    analysis_callback(adc[0], brt);
    set_all_chans(brt);

    if (tock_us(ts) > PERIOD_US)
      overflow();

    while (tock_us(ts) < PERIOD_US);

  }
  return 0;
}
