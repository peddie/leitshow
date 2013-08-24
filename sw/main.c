#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <libopencm3/stm32/f4/gpio.h>
#include <libopencm3/stm32/f4/rcc.h>
#include "./timer.h"

static void setup_peripherals(void) {
  // Setup the relevant clocks
  rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]);
  timer_setup();
  // enable FPU
  unsigned int *cpacr = (unsigned int *) 0xE000ED88;
  *cpacr |= (0xf << 20);


  // Setup the pins so that we can toggle them at will!
  // LED channels = PA0..3
  rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPAEN);
  gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                  GPIO0 | GPIO1 | GPIO2 | GPIO3);


}

int main(void) {

  setup_peripherals();

  while (1) {
    gpio_set(GPIOA, GPIO0 | GPIO1 | GPIO2 | GPIO3);
    _delay_ms(100);
    gpio_clear(GPIOA, GPIO0 | GPIO1 | GPIO2 | GPIO3);
    _delay_ms(100);
  }
  return 0;
}
