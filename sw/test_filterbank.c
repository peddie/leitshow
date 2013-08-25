#include <stdio.h>
#include <math.h>

#include "./filterbank.h"

int
main(int argc __attribute__((unused)), char **argv __attribute__((unused))) {
  filter_setup();

  float time = 0;
  float output[NUM_BANDS];
  for (int i = 0; i < 2222222; i++, time += 1.0 / 11025.0) {
    filter_step(0.5 + sin(100 * time) / 2.5, output);
    for (int j = 0; j < NUM_BANDS; j++) printf("%f ", output[j]);
    printf("\n");
  }
  return 0;
}
