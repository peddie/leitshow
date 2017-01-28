/* Copyright 2013 Matt Peddie
 * All rights reversed!
 *
 * Handy utilities for any algorithm */

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "./util.h"
#include "./config.h"

#ifdef DEBUG
#define DBGPRINT(format, ...) fprintf(stderr, __VA_ARGS__)
#else
#define DBGPRINT(format, ...)
#endif

void
create_bins(float bins[NUM_BINS],
            const float data[REAL_FFT_SIZE],
            const int boundaries[NUM_BIN_BOUNDS]) {
  int i, j;
  for (i = 0, j = 0; i < REAL_FFT_SIZE; i++) {
    if (i > boundaries[j]) {
      /* Next bin! */
      if (0 == j)
        bins[j] = bins[j] / boundaries[j];
      else
        bins[j] = bins[j] / (boundaries[j] - boundaries[j-1]);
      j++;
    }

    bins[j] += data[i];
  }
}

static inline float filter_channel(const float channel, const float channel_old,
                                   const float cutoff_hz) {
  return channel*CHAN_GAIN_FILTER_CONSTANT*cutoff_hz
      + (1 - cutoff_hz*CHAN_GAIN_FILTER_CONSTANT)*channel_old;
}

void gain_adjust_channels(float channels[NUM_OUTPUTS],
                          float filter_state[NUM_OUTPUTS],
                          float gains[NUM_OUTPUTS]) {
  /* Constants from configuration file */
  const float cutoffs[NUM_OUTPUTS] = CHAN_GAIN_FILTER_CUTOFF_HZ;
  const float act_goal[NUM_OUTPUTS] = CHAN_GAIN_GOAL_ACTIVITY;

  fprintf(stderr, "{");
  /* Scale each bin's signal by the corresponding gain. */
  for (int i = 0; i < NUM_OUTPUTS; i++) {
    channels[i] *= gains[i];

    /* Update our estimate of this channel's average power. */
    filter_state[i] = filter_channel(channels[i], filter_state[i], cutoffs[i]);

    /* Update the channel gain by feedback */
    float update = (act_goal[i] - filter_state[i]) / fabs(filter_state[i]);

    /* If we're saturating or not using a channel, get out of there! */
    float edgegain = 1;
    if (channels[i] > 1 || channels[i] <= 0)
      edgegain = 50;

    if (fabs(update) > CHAN_GAIN_UPDATE_BUMP)
      gains[i] += CHAN_GAIN_BUMP*update*edgegain;
    if (gains[i] > CHAN_GAIN_MAX) gains[i] = CHAN_GAIN_MAX;
#ifdef DEBUG
    fprintf(stderr, "%d(%.2f:%.2f::%.3f:%.2f) ",
            i, channels[i], filter_state[i], update, gains[i]);
#else
    fprintf(stderr, "%.2f ", gains[i]);
#endif  /* DEBUG */
  }
  fprintf(stderr, "\b}    ");
}

static inline float
channel_above_threshold(const float channel, const float threshold) {
  if (channel < threshold)
    return 0;
  return (channel - threshold) * (1 / (1 - threshold));
}

static inline float
binary_act_filter(const float bin, const float threshold,
                  const float bin_old, const float cutoff_hz) {
  if (bin < threshold)
    return (1 - cutoff_hz*THRESH_FILTER_CONSTANT)*bin_old;
  return CHAN_GAIN_FILTER_CONSTANT*cutoff_hz
      + (1 - cutoff_hz*CHAN_GAIN_FILTER_CONSTANT)*bin_old;
}

void
threshold_channels(float channels[NUM_OUTPUTS],
                   float filter_state[NUM_OUTPUTS],
                   float thresholds[NUM_OUTPUTS]) {
  const float cutoffs[NUM_OUTPUTS] = THRESH_FILTER_CUTOFF_HZ;
  const float goal[NUM_OUTPUTS] = THRESH_GOAL_ACTIVITY;

  fprintf(stderr, "[");
  for (int i = 0; i < NUM_OUTPUTS; i++) {
    /* Threshold the bin */
    channels[i] = channel_above_threshold(channels[i], thresholds[i]);

    /* Low-pass filter on/off state */
    filter_state[i] = binary_act_filter(channels[i], thresholds[i],
                                        filter_state[i], cutoffs[i]);

    /* Adjust threshold by feedback */
    const float update = (goal[i] - filter_state[i]) / fabs(filter_state[i]);

    if (fabs(update) > THRESH_UPDATE_BUMP)
      thresholds[i] -= THRESH_BUMP*update;

    if (thresholds[i] > THRESH_MAX)
      thresholds[i] = THRESH_MAX;
    else if (thresholds[i] < THRESH_MIN)
      thresholds[i] = THRESH_MIN;

    fprintf(stderr, " %.2f", thresholds[i]);
  }
  fprintf(stderr, "]   ");
}

void
diff_bins(float bins[NUM_BINS],
          const float filter_state[NUM_BINS]) {
  fprintf(stderr, "(");
  for (int i = 0; i < NUM_BINS; i++) {
    if (i == DECORR_BASE_BIN) {
      fprintf(stderr, "%.2e ", bins[i]);
      continue;
    }

    const float ratio = filter_state[i] / filter_state[DECORR_BASE_BIN];
    /* Alternate way to compute ratio */
    /* const float act[NUM_BINS] = CHAN_GAIN_GOAL_ACTIVITY; */
    /* const float ratio2 = act[DECORR_BASE_BIN] / act[i];  */
    bins[i] = fabs(bins[i] - filter_state[i]
                   - 0.1 * filter_state[DECORR_BASE_BIN]);
    bins[i] = DECORR_PERCENT_DERIV * fabs(bins[i] - filter_state[i])
        + (1 - DECORR_PERCENT_DERIV)
        * fabs(bins[i] - ratio * bins[DECORR_BASE_BIN]);
    fprintf(stderr, "%.2e ", bins[i]);
  }
  fprintf(stderr, "\b)    ");
}

void
clip_and_convert_channels(uint8_t converted[NUM_OUTPUTS],
                          const float inputs[NUM_OUTPUTS]) {
  int i;
  uint8_t tmp;
  float out;
  fprintf(stderr, "\t\t");
  for (i = 0; i < NUM_OUTPUTS; i++) {
    /* Make sure we don't exceed 255.  */
    // out = bins[NUM_OUTPUTS-1-i];
    out = inputs[i];
    if (out > 1.0)
      out = 1.0;
    converted[i] = (uint8_t) (255 * out);
    fprintf(stderr, "%d ", converted[i]);
  }
  fprintf(stderr, "\n");
  tmp = converted[0];
  converted[0] = converted[2];
  converted[2] = tmp;
}
