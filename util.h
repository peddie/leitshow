/* The Oasis Leitshow, handy utilities for any algorithm */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdint.h> 

#include "config.h"

/* Given an array of data and an array of boundaries between bins,
 * create normalized bin values.  */
void create_bins(float bins[NUM_CHANNELS],
                 const float data[REAL_FFT_SIZE],
                 const int boundaries[NUM_CHANNELS-1]);

/* Convert channel values from floats to uint8_ts, making sure to
 * saturate properly.  */
void clip_and_convert_channels(uint8_t channel[NUM_CHANNELS], 
                               const float bins[NUM_CHANNELS]);

#endif // __UTIL_H__
