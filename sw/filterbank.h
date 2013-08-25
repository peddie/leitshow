#ifndef __LEITSHOW_FILTERBANK_H__
#define __LEITSHOW_FILTERBANK_H__

/* This code runs the harmonic filter bank.  You put in a series of
 * audio samples.  It returns filter bank values */

/* Some filter math */
#define NUM_BANDS 9
/* Don't need to filter the highest band if we assume it goes up to
   22.05 kHz (Nyquist) */
#define NUM_FILTERS (NUM_BANDS - 1)

/* Use two low-pass filters in a row to get steeper roll-off */
#define FILTER_ORDER 4
#define NUM_BIQUADS (FILTER_ORDER / 2)
#define NUM_COEFFS (NUM_BIQUADS * 5)
#define STATE_SIZE (NUM_BIQUADS * 2)

/* Don't do anything */
void filter_setup(void);
/* Run `count` samples through the filter bank at once. `input` should
 * contain `count` samples; `output` should have room for `NUM_BANDS`
 * output values. */
void filter_step(float input, float output[]);

#endif  /* __LEITSHOW_FILTERBANK_H__ */
