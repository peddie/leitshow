#include "./leitshow.h"
#include "./filterbank.h"
#include "./analysis.h"


void leitshow_init() {
  filter_setup();
}

void leitshow_step(uint16_t sample, float output[4]) {
  analysis_callback(sample / 4095.0, output);
}
