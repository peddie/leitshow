/* The Oasis Leitshow */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>

#include <fftw3.h>

#include <termios.h>
#include <fcntl.h>

/* Input config */
#define BUFSIZE 1024
#define NUM_AUDIO_CHANNELS 1
#define AUDIO_SIZE BUFSIZE * NUM_AUDIO_CHANNELS

/* FFT config */
#define AUDIO_BYTES AUDIO_SIZE * sizeof(float)
#define REAL_FFT_SIZE (AUDIO_SIZE / 2) + 1

/* Output config */
#define NUM_CHANNELS 3
#define BIN_SHRINKAGE 3
#define FREQS_PER_CHANNEL ((int) ((float) REAL_FFT_SIZE) / 3 / BIN_SHRINKAGE)
#define FREQS_IN_TOP_CHANNEL ((REAL_FFT_SIZE % NUM_CHANNELS) / BIN_SHRINKAGE)

/* Serial I/O config */
#define BAUDRATE B115200
#define LIGHT_SHOW_DEVICE "/dev/ttyUSB0"

#define TTY_CTRL_OPTS (CS8 | CLOCAL | CREAD)
#define TTY_INPUT_OPTS IGNPAR 
#define TTY_OUTPUT_OPTS 0
#define TTY_LOCAL_OPTS 0

#define MAGIC_CODE 0xDEADBEEF

int 
main(int argc __attribute__((unused)), 
     char*argv[] __attribute__((unused))) 
{
  /* The sample type to use */
  static const pa_sample_spec ss = {
    .format = PA_SAMPLE_FLOAT32NE,
    .rate = 44100,
    .channels = NUM_AUDIO_CHANNELS
  };
  pa_simple *s = NULL;
  int ret = 1;
  int error;
  unsigned int i, j, k;
  uint8_t ser[4+NUM_CHANNELS];

  float magnitude[REAL_FFT_SIZE];
  float phase[REAL_FFT_SIZE];

  /* Create the recording stream */
  if (!(s = pa_simple_new(NULL, argv[0], PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error))) {
    fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
    goto finish;
  }

  /* Set up serial I/O */
  int rc;
  int serial = open(LIGHT_SHOW_DEVICE, O_RDWR | O_NOCTTY);
  if (serial < 0) { 
    fprintf(stderr, "Can't open serial port '%s', %s\n", LIGHT_SHOW_DEVICE, strerror(errno));
    return -1;
  }
  struct termios tio;
  memset(&tio, 0, sizeof(tio));
  tio.c_cflag = TTY_CTRL_OPTS;
  tio.c_iflag = TTY_INPUT_OPTS;
  tio.c_oflag = TTY_OUTPUT_OPTS;
  tio.c_lflag = TTY_LOCAL_OPTS;
  tio.c_cc[VTIME] = 0;     // inter-character timer unused 
  tio.c_cc[VMIN] = 1;     // blocking read until 1 character arrives 
  cfmakeraw(&tio);
  cfsetispeed(&tio, BAUDRATE);
  cfsetospeed(&tio, BAUDRATE);
  tcflush(serial, TCIFLUSH);

  if ((rc = tcsetattr(serial,TCSANOW,&tio)) < 0) {
    fprintf(stderr, "Unable to set TTY attributes, tcsetattr returned %u\n", rc);
    return -2;
  }    
  
  /* Set up the FFT stuff */
  uint8_t *buf = fftwf_malloc(AUDIO_BYTES);
  fftwf_complex *out = fftwf_malloc(sizeof(fftwf_complex) * REAL_FFT_SIZE);
  fftwf_plan plan = fftwf_plan_dft_r2c_1d(AUDIO_SIZE, (float *) buf, out, FFTW_MEASURE);

  float bins[NUM_CHANNELS];
  float bins_old[NUM_CHANNELS];
  for (j=0; j<NUM_CHANNELS; j++) bins_old[j] = 0;
  fprintf(stderr, "sizeof(buf) = %d, AUDIO_BYTES = %d, AUDIO_SIZE = %d\n",sizeof(buf), AUDIO_BYTES, AUDIO_SIZE);

  for (;;) {

    /* Record some data ... */
    if (pa_simple_read(s, buf, AUDIO_BYTES, &error) < 0) {
      fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
      goto finish;
    }

    /* Get the Fourier transform of this chunk */
    fftwf_execute(plan);
    
    for (j=0; j<NUM_CHANNELS; j++) {
      bins[j] = 0;
    }
    /* Break it down(!) into magnitude and phase. */
    // fprintf(stdout, "\n\n\n");
    for (i=0, j=0, k=0; i<REAL_FFT_SIZE; i++, j++) {
      magnitude[i] = sqrt(out[i][0]*out[i][0] + out[i][1]*out[i][1]);
      phase[i] = atan2(out[i][1], out[i][0]);
      bins[k] += 0.4 * magnitude[i];
      if (j >= FREQS_PER_CHANNEL) {j=0; k++; }
    }

    ser[0] = 0xDE;
    ser[1] = 0xAD;
    ser[2] = 0xBE;
    ser[3] = 0xEF;
    for (j=0; j<NUM_CHANNELS; j++) {
      if (j < NUM_CHANNELS-1) {
        bins[j] /= FREQS_PER_CHANNEL;
      } else {
        bins[j] /= FREQS_IN_TOP_CHANNEL;
      }
      // fprintf(stderr, "%d ", (uint8_t) (255 * (bins[j] / 3)));
      float filt = 0.1 * bins[j] + 0.9 * bins_old[j];
      ser[4+j] = (uint8_t) (255 * (filt/ 3));
      bins_old[j] = filt;
    }
    // fprintf(stderr, "\n");
    rc = write(serial, ser, 4+NUM_CHANNELS);

    /* Make sure we don't end up accidentally reusing data. */
    memset(out, 0, sizeof(fftwf_complex) * REAL_FFT_SIZE);
    memset(buf, 0, AUDIO_BYTES);
  }




  ret = 0;

finish:

  /* fftwf_free(out);   */
  /* fftwf_free(buf); */
  /* fftwf_destroy_plan(plan); */
  if (s)
    pa_simple_free(s);

  return ret;
}

