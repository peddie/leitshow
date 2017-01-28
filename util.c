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
create_bins(float bins[NUM_CHANNELS],
            const float data[REAL_FFT_SIZE],
            const int boundaries[NUM_CHANNELS-1]) {
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

static inline float
filter_bin(const float bin, const float bin_old, const float cutoff_hz) {
  return bin*CHAN_GAIN_FILTER_CONSTANT*cutoff_hz
      + (1 - cutoff_hz*CHAN_GAIN_FILTER_CONSTANT)*bin_old;
}

void
gain_adjust_bins(float bins[NUM_CHANNELS],
                 float filter_state[NUM_CHANNELS],
                 float gains[NUM_CHANNELS]) {
  /* Constants from configuration file */
  const float cutoffs[NUM_CHANNELS] = CHAN_GAIN_FILTER_CUTOFF_HZ;
  const float act_goal[NUM_CHANNELS] = CHAN_GAIN_GOAL_ACTIVITY;

  /* Scale each bin's signal by the corresponding gain. */
  for (int i = 0; i < NUM_CHANNELS; i++) {
    bins[i] *= gains[i];

    /* Update our estimate of this channel's average power. */
    filter_state[i] = filter_bin(bins[i], filter_state[i], cutoffs[i]);

    /* Update the channel gain by feedback */
    float update = (act_goal[i] - filter_state[i]) / fabs(filter_state[i]);

    /* If we're saturating or not using a channel, get out of there! */
    float edgegain = 1;
    if (bins[i] > 1 || bins[i] <= 0)
      edgegain = 50;

    if (fabs(update) > CHAN_GAIN_UPDATE_BUMP)
      gains[i] += CHAN_GAIN_BUMP*update*edgegain;
    if (gains[i] > CHAN_GAIN_MAX) gains[i] = CHAN_GAIN_MAX;
#ifdef DEBUG
    fprintf(stderr, "%d(%.2f:%.2f::%.3f:%.2f) ",
            i, bins[i], filter_state[i], update, gains[i]);
#else
    fprintf(stderr, "%.2f ", gains[i]);
#endif  /* DEBUG */
  }
  fprintf(stderr, "    ");
}

static inline float
bin_above_threshold(const float bin, const float threshold) {
  if (bin < threshold)
    return 0;
  return (bin - threshold) * (1 / (1 - threshold));
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
threshold_bins(float bins[NUM_CHANNELS],
               float filter_state[NUM_CHANNELS],
               float thresholds[NUM_CHANNELS]) {
  const float cutoffs[NUM_CHANNELS] = THRESH_FILTER_CUTOFF_HZ;
  const float goal[NUM_CHANNELS] = THRESH_GOAL_ACTIVITY;

  for (int i = 0; i < NUM_CHANNELS; i++) {
    /* Threshold the bin */
    bins[i] = bin_above_threshold(bins[i], thresholds[i]);

    /* Low-pass filter on/off state */
    filter_state[i] = binary_act_filter(bins[i], thresholds[i],
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
  fprintf(stderr, "\t");
}

void
diff_bins(float bins[NUM_CHANNELS],
          const float filter_state[NUM_CHANNELS] __attribute__((unused))) {
  for (int i = 0; i < NUM_CHANNELS; i++) {
    if (i == DECORR_BASE_CHANNEL) {
      fprintf(stderr, "%+03.2lf ", bins[i]);
      continue;
    }

    const float ratio = filter_state[i] / filter_state[DECORR_BASE_CHANNEL];
    /* Alternate way to compute ratio */
    /* const float act[NUM_CHANNELS] = CHAN_GAIN_GOAL_ACTIVITY; */
    /* const float ratio2 = act[DECORR_BASE_CHANNEL] / act[i];  */
    bins[i] = fabs(bins[i] - filter_state[i]
                   - 0.1 * filter_state[DECORR_BASE_CHANNEL]);
    bins[i] = DECORR_PERCENT_DERIV * fabs(bins[i] - filter_state[i])
        + (1 - DECORR_PERCENT_DERIV)
        * fabs(bins[i] - ratio * bins[DECORR_BASE_CHANNEL]);
    fprintf(stderr, "%+03.2lf ", bins[i]);
  }
}

void
clip_and_convert_channels(uint8_t channel[NUM_CHANNELS],
                          const float bins[NUM_CHANNELS]) {
  int i;
  uint8_t tmp;
  float out;
  fprintf(stderr, "\t\t");
  for (i = 0; i < NUM_CHANNELS; i++) {
    /* Make sure we don't exceed 255.  */
    // out = bins[NUM_CHANNELS-1-i];
    out = bins[i];
    if (out > 1.0)
      out = 1.0;
    channel[i] = (uint8_t) (255 * out);
    fprintf(stderr, "%d ", channel[i]);
  }
  fprintf(stderr, "\n");
  tmp = channel[0];
  channel[0] = channel[2];
  channel[2] = tmp;
}
