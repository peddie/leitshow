
#include "./filterbank.h"
#include "./analysis.h"

void
analyze(float input[NUM_BANDS], float output[NUM_CHANNELS]) {
  /* Simple bin chunking for now; no adaptive filters or derivatives,
   * etc. */
  const int div = NUM_BANDS / NUM_CHANNELS;
  for (int i = 0; i < NUM_CHANNELS - 1; i++) {
    output[i] = 0;
    for (int j = 0; j < div; j++)
      output[i] += input[i * div + j];
  }

  const int left = NUM_BANDS % NUM_CHANNELS;
  const int max = left ? div : left;
  const int last = NUM_CHANNELS - 1;
  output[last] = 0;
  for (int i = 0; i < max; i++)
    output[last] += input[div * last + i];
}

void
analysis_callback(float input, float output[NUM_CHANNELS]) {
  float bins[NUM_BANDS];
  filter_step(input, bins);
  analyze(bins, output);
}
