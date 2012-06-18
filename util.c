/* Oasis Leitshow, handy utilities for any algorithm */

#include <stdio.h>
#include <stdint.h>

#include "util.h"
#include "config.h"

#ifndef DEBUG
#define DBGPRINT(format, ...) fprintf(stderr, __VA_ARGS__)
#else 
#define DBGPRINT(format, ...)
#endif

void
create_bins(float bins[NUM_CHANNELS],
            const float data[REAL_FFT_SIZE],
            const int boundaries[NUM_CHANNELS-1])
{
  int i, j;
  for (i=0, j=0; i<REAL_FFT_SIZE; i++) {
    if (i > boundaries[j]) {
      /* Next bin! */
      if (j==0) bins[j] = bins[j] / boundaries[j];
      else bins[j] = bins[j] / (boundaries[j] - boundaries[j-1]);
      j++;
    }

    bins[j] += data[i];
  }
}

void
clip_and_convert_channels(uint8_t channel[NUM_CHANNELS], 
                          const float bins[NUM_CHANNELS])
{
  int i;
  uint8_t tmp;
  float out;
  for (i=0; i<NUM_CHANNELS; i++) {
    /* Make sure we don't exceed 255.  */
    // out = bins[NUM_CHANNELS-1-i];
    out = bins[i];
    if (out > 1.0)
      out = 1.0;
    channel[i] = (uint8_t) (255 * out);
/*     DBGPRINT(stderr, "%d ", channel[i]); */
  }
/*   DBGPRINT(stderr, "\n"); */
  tmp = channel[0];
  channel[0] = channel[2]; 
  channel[2] = tmp;
}


