/* Copyright 2013 Matt Peddie
 * All rights reversed!
 *
 * Handy utilities for any algorithm */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdint.h>

#include "config.h"

/* Given an array of data and an array of boundaries between bins,
 * create normalized bin values.  */
void create_bins(float bins[NUM_BINS],
                 const float data[REAL_FFT_SIZE],
                 const int boundaries[NUM_BIN_BOUNDS]);

/* Given an array of bin data, bin gains and low-pass filter states,
 * scale the bin values by the gains, then adjust the gains to make
 * the filtered bin power value match CHAN_GAIN_GOAL_ACTIVITY */
void gain_adjust_channels(float channels[NUM_OUTPUTS],
                          float filter_state[NUM_OUTPUTS],
                          float gains[NUM_OUTPUTS]);


/* This is like `gain_adjust_channels` but using a simpler and more
   logarithmic gain adaptation mechanism. */
void log_sliding_mode_gain(float channels[NUM_OUTPUTS],
                           float filter_state[NUM_OUTPUTS],
                           float gains[NUM_OUTPUTS]);

void threshold_channels(float channels[NUM_OUTPUTS],
                        float filter_state[NUM_OUTPUTS],
                        float thresholds[NUM_OUTPUTS]);

/* Given an array of bin data and low-pass filter states, make all bin
 * values except DECORR_BASE_CHANNEL the difference between that bin
 * value and DECORR_BASE_CHANNEL, normalized appropriately. */
void diff_bins(float bins[NUM_BINS],
               const float filter_state[NUM_BINS]);

/* Convert channel values from floats to uint8_ts, making sure to
 * saturate properly.  */
void clip_and_convert_channels(uint8_t channel[NUM_OUTPUTS],
                               const float bins[NUM_OUTPUTS]);

#endif  // __UTIL_H__
