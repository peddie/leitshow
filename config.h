/* The Oasis Leitshow, compile-time configuration */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <termios.h>

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
#define BIN_FILTER_CONSTANT ((BIN_FILTER_SAMPLE_TIME * 6.283185307179586) / (1 + (BIN_FILTER_SAMPLE_TIME * 6.283185307179586)))

/* Serial I/O config */
#define BAUDRATE B115200
#define LIGHT_SHOW_DEVICE "/dev/ttyUSB0"

#define TTY_CTRL_OPTS (CS8 | CLOCAL | CREAD)
#define TTY_INPUT_OPTS IGNPAR 
#define TTY_OUTPUT_OPTS 0
#define TTY_LOCAL_OPTS 0

#define MAGIC_CODE 0xDEADBEEF

#endif // __CONFIG_H__