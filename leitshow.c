/* Copyright 2013 Matt Peddie
 * All rights reversed!
 *
 * The Oasis Leitshow */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>

#include <fftw3.h>

#include <termios.h>
#include <fcntl.h>

#include "./config.h"
#include "./util.h"
#include "./pca.h"

#include "./arraymath.h"

#ifndef DEBUG
#define DBGPRINT(format, ...) fprintf(stderr, __VA_ARGS__)
#else
#define DBGPRINT(format, ...)
#endif

struct termios tio;

/* Bin boundary indices within the FFT data */
int bin_bounds[NUM_BIN_BOUNDS] = {16 * BUFSIZE / 2048,  32 * BUFSIZE / 2048,
                                  64 * BUFSIZE / 2048,  128 * BUFSIZE / 2048,
                                  256 * BUFSIZE / 2048, 512 * BUFSIZE / 2048,
                                  2048 * BUFSIZE / 2048};

/* Power values for each channel at the last timestep */
float channels_old[NUM_OUTPUTS];

/* Activity filter state */
float filter_state[NUM_OUTPUTS] = CHAN_GAIN_GOAL_ACTIVITY;
/* Channel gains */
float gains[NUM_OUTPUTS] = CHANNEL_GAIN;

/* Threshold filter state */
float thresh_filter_state[NUM_OUTPUTS] = THRESH_GOAL_ACTIVITY;
/* Activity thresholds */
float thresholds[NUM_OUTPUTS];

SampleBuffer *sample_buffer = NULL;

static int
serial_setup(const char *device) {
  int rc;
  int serial = open(device, O_RDWR | O_NOCTTY);
  if (serial < 0) {
    fprintf(stderr,
            "Can't open serial port '%s', %s\n", device, strerror(errno));
    return -1;
  }
  memset(&tio, 0, sizeof(tio));
  tio.c_cflag = TTY_CTRL_OPTS;
  tio.c_iflag = TTY_INPUT_OPTS;
  tio.c_oflag = TTY_OUTPUT_OPTS;
  tio.c_lflag = TTY_LOCAL_OPTS;
  tio.c_cc[VTIME] = 0;
  tio.c_cc[VMIN] = 1;
  cfmakeraw(&tio);
  cfsetispeed(&tio, BAUDRATE);
  cfsetospeed(&tio, BAUDRATE);
  tcflush(serial, TCIFLUSH);

  if ((rc = tcsetattr(serial, TCSANOW, &tio)) < 0) {
    fprintf(stderr, "Unable to set TTY attributes, tcsetattr returned %u\n",
            rc);
    return -2;
  }

  return serial;
}

static inline void
calc_fft_stats(const fftwf_complex freq_data[REAL_FFT_SIZE],
               float freq_powers[REAL_FFT_SIZE],
               float freq_phases[REAL_FFT_SIZE]) {
  for (int i = 0; i < REAL_FFT_SIZE; i++) {
    freq_powers[i] = sqrt(freq_data[i][0]*freq_data[i][0]
                          + freq_data[i][1]*freq_data[i][1]);
    freq_phases[i] = atan2(freq_data[i][1], freq_data[i][0]);
  }
}

static void
calc_stats(const fftwf_complex freq_data[REAL_FFT_SIZE],
           float bin_powers[NUM_BINS],
           float bin_phases[NUM_BINS]) {
  int i;
  float freq_powers[REAL_FFT_SIZE], freq_phases[REAL_FFT_SIZE];
  calc_fft_stats(freq_data, freq_powers, freq_phases);

  for (i = 0; i < NUM_BINS; i++) {
    /* Compute safe indices */
    int min_ind, max_ind, bin_size;
    if (i == 0)
      min_ind = 0;
    else
      min_ind = bin_bounds[i - 1];
    if (i == NUM_BINS - 1)
      max_ind = REAL_FFT_SIZE - 1;
    else
      max_ind = bin_bounds[i] - 1;

    bin_size = max_ind - min_ind;

    /* Loop over all FFT data within this bin and compute magnitude
     * and phase. */
    bin_powers[i] = bin_phases[i] = 0;
    bin_powers[i] = array_sum(&freq_powers[min_ind], bin_size);
    bin_phases[i] = array_sum(&freq_phases[min_ind], bin_size);
  }
}

static void
calc_bins(float bins[NUM_BINS],
          const float time_data[AUDIO_SIZE] __attribute__((unused)),
          const fftwf_complex freq_data[REAL_FFT_SIZE]) {
  /* Compute binwise statistics */
  float bin_phases[NUM_BINS];
  calc_stats(freq_data, bins, bin_phases);
}

static inline void
lpf_channels(float bins[NUM_OUTPUTS]) {
  /* Low-pass filter each channel (set the performance with
   * BIN_FILTER_CUTOFF_HZ).  */
  const float binfilter[NUM_OUTPUTS] = BIN_FILTER_CUTOFF_HZ;
  for (int i = 0; i < NUM_OUTPUTS; i++)
    channels_old[i] = bins[i] =
        binfilter[i] * BIN_FILTER_CONSTANT * bins[i] +
        (1 - binfilter[i] * BIN_FILTER_CONSTANT) * channels_old[i];
}

float float_channel_old[NUM_OUTPUTS];

static void
set_channels(uint8_t channel[NUM_OUTPUTS],
             const float time_data[AUDIO_SIZE],
             const fftwf_complex freq_data[REAL_FFT_SIZE]) {
  float bins[NUM_BINS];
  /* Make sure we don't end up accidentally reusing data. */
  for (int i = 0; i < NUM_BINS; i++) bins[i] = 0;

  /* Calculate values based on audio */
  calc_bins(bins, time_data, freq_data);

  float total_bins = 0;
  for (int i = 0; i < NUM_BINS; i++) total_bins += fabsf(bins[i]);

  float float_channel[NUM_OUTPUTS];
  if (total_bins < 1e-5) {
    /* Not enough signal; use the data from the previous timestep. */
    memcpy(float_channel, float_channel_old, sizeof(float_channel));
  } else {
    /* Got signal -- analyze! */

    float binout[NUM_BINS];
    activate_samples(NUM_BINS, bins, sample_buffer, binout);
    for (int i = 0; i < NUM_BINS; i++) {
      float_channel[i % NUM_OUTPUTS] += fabsf(binout[i]);
      // fabsf(logf(fabsf(binout[i]) + 1e-5) + 6);
    }
    fprintf(stderr, "(");
    for (int i = 0; i < NUM_OUTPUTS; i++) {
      fprintf(stderr, "%.2e ", float_channel[i]);
    }
    fprintf(stderr, "\b)  ");
    /* Differentiate most bins to decorrelate them */
    /* diff_bins(bins, filter_state); */
    /* memcpy(float_channel, bins, sizeof(float_channel)); */

    /* Adjust for better activity */
    gain_adjust_channels(float_channel, filter_state, gains);
    /* log_sliding_mode_gain(float_channel, filter_state, gains); */

    /* Low-pass output signals */
    lpf_channels(float_channel);

    /* Threshold for better activity */
    threshold_channels(float_channel, thresh_filter_state, thresholds);
    memcpy(float_channel_old, float_channel, sizeof(float_channel_old));
  }

  /* Convert to 8-bit binary */
  clip_and_convert_channels(channel, float_channel);
}


int
main(int argc __attribute__((unused)),
     char*argv[] __attribute__((unused))) {
  /* The sample type to use */
  static const pa_sample_spec ss = {
    .format = PA_SAMPLE_FLOAT32NE,
    .rate = AUDIO_SAMPLE_RATE,
    .channels = NUM_AUDIO_CHANNELS
  };

  static const pa_buffer_attr ba = {
    /* This is the only one that matters for us in playback.  This
     * gives 3-4 ms of latency on a core i5 laptop. */
    .fragsize = 2222,
    .maxlength = (uint32_t) -1,         /* Accept server defaults */
    .minreq = (uint32_t) -1,
    .prebuf = (uint32_t) -1,
    .tlength = (uint32_t) -1,
  };
  pa_simple *s = NULL;
  int error, serial;
  uint8_t ser[4+NUM_OUTPUTS];

  /* Create the recording stream */
  if (!(s = pa_simple_new(NULL, argv[0], PA_STREAM_RECORD, NULL, "record", &ss,
                          NULL, &ba, &error))) {
    fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n",
            pa_strerror(error));
    goto finish;
  }

  /* Set up serial I/O */
  if ((serial = serial_setup(LIGHT_SHOW_DEVICE)) ==22) exit(1);
  /* Write the magic code word */
  ser[0] = 0xDE;
  ser[1] = 0xAD;
  ser[2] = 0xBE;
  ser[3] = 0xEF;

  /* Set up the FFT stuff */
  float *buf = fftwf_malloc(AUDIO_BYTES * BUFFER_CYCLE);
  fftwf_complex *out = fftwf_malloc(sizeof(fftwf_complex) * REAL_FFT_SIZE);
  fftwf_plan plan = fftwf_plan_dft_r2c_1d(AUDIO_SIZE*BUFFER_CYCLE,
                                          buf, out, FFTW_MEASURE);

  sample_buffer = new_sample_buffer(PCA_HISTORY, NUM_BINS);
  if (NULL == sample_buffer) {
    fprintf(stderr, "Failed to allocate sample buffer, sorry.\n");
    return 1;
  }
  memset(buf, 0, AUDIO_BYTES*BUFFER_CYCLE);
  fprintf(stderr,
          "AUDIO_BYTES = %zu, AUDIO_SIZE = %d, REAL_FFT_SIZE = %d, "
          "BIN_FILTER_CONSTANT = %f\n",
          AUDIO_BYTES, AUDIO_SIZE, REAL_FFT_SIZE,
          BIN_FILTER_CONSTANT);
  fprintf(stderr, "DT = %lf, CHAN_GAIN_FILTER_CONSTANT = %lf\n",
          ((double)BUFSIZE)/((double)AUDIO_SAMPLE_RATE),
          CHAN_GAIN_FILTER_CONSTANT);

  while (1) {
    /* Cycle audio data through the buffer */
    for (int i = BUFFER_CYCLE-2; i >= 0; i--)
      memcpy(&buf[AUDIO_SIZE*(i+1)], &buf[AUDIO_SIZE*i], AUDIO_BYTES);

    /* Record some data ... */
    if (pa_simple_read(s, (uint8_t *) buf, AUDIO_BYTES, &error) < 0) {
      fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n",
              pa_strerror(error));
      goto finish;
    }

    /* Get the Fourier transform of this chunk */
    fftwf_execute(plan);

    /* Map the FFT data onto channel magnitudes. */
    set_channels(&ser[4], buf, (const fftwf_complex *) out);

    error = write(serial, ser, 4+NUM_OUTPUTS);
  }

 finish:
  /* fftwf_free(out);   */
  /* fftwf_free(buf); */
  /* fftwf_destroy_plan(plan); */
  if (s)
    pa_simple_free(s);

  return 0;
}
