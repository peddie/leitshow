/* The Oasis Leitshow */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>

#include <fftw3.h>

#include <termios.h>
#include <fcntl.h>

#ifndef DEBUG
#define DBGPRINT(format, ...) fprintf(stderr, __VA_ARGS__)
#else 
#define DBGPRINT(format, ...)
#endif

/* FFT config */
#define BUFFER_CYCLE 2
#define REAL_FFT_SIZE ((((int) ((float) AUDIO_SIZE / 2)) * BUFFER_CYCLE))

/* Input config */
#define BUFSIZE 2048
#define NUM_AUDIO_CHANNELS 1
#define AUDIO_SIZE BUFSIZE * NUM_AUDIO_CHANNELS
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_BYTES AUDIO_SIZE * sizeof(float)

/* Output config */
#define NUM_CHANNELS 4

/* We divide our number of frequency samples into this many chunks and
 * then use only the lowest-frequency chunk.  */
#define BIN_SHRINKAGE 2.3

/* This is a fixed scaling number, but really each channel should
 * slowly adapt its outputs to the amount of power in it. */
#define OUTPUT_PRESCALE 0.01

#define FREQS_PER_CHANNEL ((int) (((float) REAL_FFT_SIZE) / 3 / BIN_SHRINKAGE))
#define FREQS_IN_TOP_CHANNEL ((int) (REAL_FFT_SIZE % NUM_CHANNELS) / BIN_SHRINKAGE)

/* Filtering config */
#define BIN_FILTER_CUTOFF_HZ {1, 1.5, 3, 4}
#define BIN_FILTER_SAMPLE_TIME (((float) BUFSIZE) / ((float) AUDIO_SAMPLE_RATE))
#define BIN_FILTER_CONSTANT (BIN_FILTER_SAMPLE_TIME * 6.283185307179586)

/* Serial I/O config */
#define BAUDRATE B115200
#define LIGHT_SHOW_DEVICE "/dev/ttyUSB0"

#define TTY_CTRL_OPTS (CS8 | CLOCAL | CREAD)
#define TTY_INPUT_OPTS IGNPAR 
#define TTY_OUTPUT_OPTS 0
#define TTY_LOCAL_OPTS 0

#define MAGIC_CODE 0xDEADBEEF

struct termios tio;

int
serial_setup(const char *device)
{
  int rc;
  int serial = open(device, O_RDWR | O_NOCTTY);
  if (serial < 0) { 
    fprintf(stderr, "Can't open serial port '%s', %s\n", device, strerror(errno));
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

  if ((rc = tcsetattr(serial,TCSANOW,&tio)) < 0) {
    fprintf(stderr, "Unable to set TTY attributes, tcsetattr returned %u\n", rc);
    return -2;
  }    

  return serial;
}

void
create_bins(float bins[NUM_CHANNELS], 
            const float time_data[AUDIO_SIZE] __attribute__((unused)),
            const fftwf_complex freq_data[REAL_FFT_SIZE])
{
  int i, j, k;
  float magnitude; //, phase, power;
  static float bins_old[NUM_CHANNELS];
  float binfilter[NUM_CHANNELS] = BIN_FILTER_CUTOFF_HZ;

  /* Calculate the power of this audio sample */
  /* for (i=0, power=0; i<AUDIO_SIZE; i++) power += time_data[i]*time_data[i]; */
  /* power = sqrt(power) / AUDIO_SIZE; */
    
  for (i=0, j=0, k=0; i<REAL_FFT_SIZE; i++, j++) {
    /* Break it down into magnitude and phase. */
    magnitude = sqrt(freq_data[i][0]*freq_data[i][0] 
                     + freq_data[i][1]*freq_data[i][1]);
    // phase = atan2(freq_data[i][1], freq_data[i][0]);
    if (k < NUM_CHANNELS) {
      bins[k] += OUTPUT_PRESCALE * magnitude;
      if (j >= FREQS_PER_CHANNEL) {j=0; k++;}
    } else break;
  }

  /* Normalize bin magnitudes. */
  for (k=0; k<NUM_CHANNELS; k++) {
    if ((k == NUM_CHANNELS-1) && (BIN_SHRINKAGE == 1)) 
      bins[k] /= FREQS_IN_TOP_CHANNEL;
    else
      bins[k] /= FREQS_PER_CHANNEL;

    /* Low-pass filter each channel (set the performance with
     * BIN_FILTER_CUTOFF_HZ).  */
    bins_old[k] = bins[k] = binfilter[k]*BIN_FILTER_CONSTANT * bins[k] * (1 + 2 * (k>1))
      + (1 - binfilter[k]*BIN_FILTER_CONSTANT) * bins_old[k];
    DBGPRINT(stderr, "%f ", bins[k]);
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
    DBGPRINT(stderr, "%d ", channel[i]);
  }
  DBGPRINT(stderr, "\n");
  tmp = channel[0];
  channel[0] = channel[2]; 
  channel[2] = tmp;
}

void
set_channels(uint8_t channel[NUM_CHANNELS], 
             const float time_data[AUDIO_SIZE], 
             const fftwf_complex freq_data[REAL_FFT_SIZE])
{
  int i;
  float bins[NUM_CHANNELS];
  /* Make sure we don't end up accidentally reusing data. */
  for (i=0; i<NUM_CHANNELS; i++) bins[i]=0;

  create_bins(bins, time_data, freq_data);
  clip_and_convert_channels(channel, bins);
}


int 
main(int argc __attribute__((unused)), 
     char*argv[] __attribute__((unused))) 
{
  /* The sample type to use */
  static const pa_sample_spec ss = {
    .format = PA_SAMPLE_FLOAT32NE,
    .rate = AUDIO_SAMPLE_RATE,
    .channels = NUM_AUDIO_CHANNELS
  };
  pa_simple *s = NULL;
  int error, serial, i;
  uint8_t ser[4+NUM_CHANNELS];

  /* Create the recording stream */
  if (!(s = pa_simple_new(NULL, argv[0], PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error))) {
    fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
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
  fftwf_plan plan = fftwf_plan_dft_r2c_1d(AUDIO_SIZE*BUFFER_CYCLE, buf, out, FFTW_MEASURE);

  memset(buf, 0, AUDIO_BYTES*BUFFER_CYCLE);
  fprintf(stderr, 
          "AUDIO_BYTES = %d, AUDIO_SIZE = %d, REAL_FFT_SIZE = %d, BIN_FILTER_CONSTANT = %f, FREQS_PER_CHANNEL = %d\n", 
          AUDIO_BYTES, AUDIO_SIZE, REAL_FFT_SIZE, BIN_FILTER_CONSTANT, FREQS_PER_CHANNEL);

  for (;;) {

    /* Cycle audio data through the buffer */
    for (i=BUFFER_CYCLE-2; i>=0; i--)
      memcpy(&buf[AUDIO_SIZE*(i+1)], &buf[AUDIO_SIZE*i], AUDIO_BYTES);

    /* Record some data ... */
    if (pa_simple_read(s, (uint8_t *) buf, AUDIO_BYTES, &error) < 0) {
      fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
      goto finish;
    }

    /* Get the Fourier transform of this chunk */
    fftwf_execute(plan);

    /* Map the FFT data onto channel magnitudes. */
    set_channels(&ser[4], buf, (const fftwf_complex *) out);

    error = write(serial, ser, 4+NUM_CHANNELS);
  }

finish:

  /* fftwf_free(out);   */
  /* fftwf_free(buf); */
  /* fftwf_destroy_plan(plan); */
  if (s)
    pa_simple_free(s);

  return 0;
}

