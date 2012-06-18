#ifndef __NEW_ADAP_H__
#define __NEW_ADAP_H__

void adap_decorr_init(void);

void
bin_bound_adjust(int bin_bounds[NUM_BOUNDS],
                 const float bin_powers[NUM_CHANNELS],
                 const float freq_powers[REAL_FFT_SIZE],
                 const fftwf_complex freq_data[REAL_FFT_SIZE]);

#endif  /* __NEW_ADAP_H__ */
