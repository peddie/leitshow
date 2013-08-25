#include "./leitshow.h"

void leitshow_init() {
}

void leitshow_step(uint16_t sample, float output[4]) {
  static uint16_t prev;

  output[0] = (sample > (prev + 10));
  output[3] = output[2] = output[1] = output[0];

  prev = sample;
}
