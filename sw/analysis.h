#ifndef __LEITSHOW_ANALYSIS_H__
#define __LEITSHOW_ANALYSIS_H__

#include "./filterbank.h"

#define NUM_CHANNELS 4
void analyze(float input[NUM_BANDS], float output[NUM_CHANNELS]);
void analysis_callback(float input, float output[NUM_CHANNELS]);

#endif  /* __LEITSHOW_ANALYSIS_H__ */
