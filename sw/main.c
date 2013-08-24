#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <libopencm3/stm32/f4/gpio.h>
#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/f4/timer.h>
#include "./timer.h"
#include "./pwm.h"

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



static void setup_peripherals(void) {
  // Setup the relevant clocks
  rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]);
  timer_setup();
  // enable FPU
  unsigned int *cpacr = (unsigned int *) 0xE000ED88;
  *cpacr |= (0xf << 20);

  pwm_setup();
}

static void set_all_chans(float percent[N_CHANS]) {
  pwm_set_duty(PWM_TIMER,PWM_TIM_CHAN_RED, percent[0]);
  pwm_set_duty(PWM_TIMER,PWM_TIM_CHAN_GREEN, percent[1]);
  pwm_set_duty(PWM_TIMER,PWM_TIM_CHAN_BLUE, percent[2]);
  pwm_set_duty(PWM_TIMER,PWM_TIM_CHAN_WHITE, percent[3]);
}

int main(void) {

  setup_peripherals();

  float brt[N_CHANS] = {0};

  float theta = 0;

  while (1) {
    for (brt[BLUE] = 0; brt[BLUE] <= 1; brt[BLUE] += 0.01) {
      set_all_chans(brt);
      _delay_ms(50);
    }
  }
  return 0;
}
