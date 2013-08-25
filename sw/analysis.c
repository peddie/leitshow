/* Copyright 2013 Matthew Peddie <peddie@alum.mit.edu> */
#include <math.h>
#include <string.h>
#include <stdint.h>

#include "./analysis.h"

#define GLOBAL_SAMPLE_TIME (1.0 / 11025.0)
#define MIN_SPECTRUM_POWER 1e-8

/* Envelope filtering config */
#define CHANNEL_GAIN {0.3, 1.5, 1.5, 0.5}
#define BIN_FILTER_CUTOFF_HZ {3, 5, 5, 3}
#define BIN_FILTER_SAMPLE_TIME GLOBAL_SAMPLE_TIME
#define BIN_FILTER_CONSTANT ((BIN_FILTER_SAMPLE_TIME * 6.283185307179586) / \
(1 + (BIN_FILTER_SAMPLE_TIME * 6.283185307179586)))

/* channel gain adaptation config */
#define CHAN_GAIN_FILTER_CUTOFF_HZ {0.02, 0.02, 0.02, 0.02}
#define CHAN_GAIN_FILTER_SAMPLE_TIME GLOBAL_SAMPLE_TIME
#define CHAN_GAIN_FILTER_CONSTANT \
((CHAN_GAIN_FILTER_SAMPLE_TIME * 6.283185307179586) / \
(1 + (CHAN_GAIN_FILTER_SAMPLE_TIME * 6.283185307179586)))
#define CHAN_GAIN_MAX 22.0
#define CHAN_GAIN_GOAL_ACTIVITY {0.3, 0.3, 0.3, 0.5}
#define CHAN_GAIN_BUMP 0.001
#define CHAN_GAIN_UPDATE_BUMP 0.05

/* Threshold config */
#define THRESH_GOAL_ACTIVITY {0.3, 0.3, 0.3, 0.1}
#define THRESH_FILTER_CUTOFF_HZ {0.04, 0.04, 0.04, 0.04}
#define THRESH_FILTER_SAMPLE_TIME GLOBAL_SAMPLE_TIME
#define THRESH_FILTER_CONSTANT \
((THRESH_FILTER_SAMPLE_TIME * 6.283185307179586) / \
(1 + (THRESH_FILTER_SAMPLE_TIME * 6.283185307179586)))
#define THRESH_MIN 0.02
#define THRESH_MAX 0.9
#define THRESH_BUMP 0.1
#define THRESH_UPDATE_BUMP 0.05

/* Power values for each channel at the last timestep */
float bins_old[NUM_CHANNELS] = {0};

static inline void
lpf_bins(float bins[NUM_CHANNELS]) {
  /* Low-pass filter each channel (set the performance with
   * BIN_FILTER_CUTOFF_HZ). */
  const float binfilter[NUM_CHANNELS] = BIN_FILTER_CUTOFF_HZ;
  for (int i = 0; i < NUM_CHANNELS; i++)
    bins_old[i] = bins[i] = binfilter[i]*BIN_FILTER_CONSTANT * bins[i]
        + (1 - binfilter[i]*BIN_FILTER_CONSTANT) * bins_old[i];
}

static inline float
filter_bin(const float bin, const float bin_old, const float cutoff_hz) {
  return bin*CHAN_GAIN_FILTER_CONSTANT*cutoff_hz
      + (1 - cutoff_hz*CHAN_GAIN_FILTER_CONSTANT)*bin_old;
}
static void
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
  }
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

static void
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
  }
}

float gain_filter_states[NUM_CHANNELS] = CHAN_GAIN_GOAL_ACTIVITY;
float gains[NUM_CHANNELS] = CHANNEL_GAIN;
float thresh_filter_state[NUM_CHANNELS] = THRESH_GOAL_ACTIVITY;
/* Activity thresholds */
float thresholds[NUM_CHANNELS] = {0};

void
analyze(float input[NUM_BANDS], float output[NUM_CHANNELS]) {
  /* Simple bin chunking for now */
  static uint32_t loopcount = 0;
  output[0] = input[2] + input[3];
  output[1] = input[4] + input[5];
  output[2] = input[6] + input[7];
  output[3] = input[8] + input[9];

  if (loopcount++ > 100000)
    output[0] += 0.00001;

  const float total_bins = output[0] + output[1] + output[2] + output[3];
  if (total_bins > MIN_SPECTRUM_POWER) {
    /* Adaptive filter on gains */
    gain_adjust_bins(output, gain_filter_states, gains);

    /* Channel filters */
    lpf_bins(output);

    /* Adaptive filter on thresholds */
    threshold_bins(output, thresh_filter_state, thresholds);
  } else {
    memcpy(output, gain_filter_states, sizeof(gain_filter_states));
  }
}

void
analysis_callback(float input, float output[NUM_CHANNELS]) {
  float bins[NUM_BANDS];
  filter_step(input, bins);

  analyze(bins, output);
}
