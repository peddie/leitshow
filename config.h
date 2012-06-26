/* The Oasis Leitshow, compile-time configuration */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <termios.h>

/* Input config */
#define BUFSIZE 2048
#define NUM_AUDIO_CHANNELS 1
#define AUDIO_SIZE BUFSIZE * NUM_AUDIO_CHANNELS
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_BYTES AUDIO_SIZE * sizeof(float)

/* FFT config */
#define BUFFER_CYCLE 2
#define REAL_FFT_SIZE ((((int) ((float) AUDIO_SIZE / 2)) * BUFFER_CYCLE))

/* Output config */
#define NUM_CHANNELS 4
#define NUM_BOUNDS (NUM_CHANNELS - 1)

/* We divide our number of frequency samples into this many chunks and
 * then use only the lowest-frequency chunk.  */
#define BIN_SHRINKAGE 2.5

/* This is a fixed scaling number, but really each channel should
 * slowly adapt its outputs to the amount of power in it. */
#define OUTPUT_PRESCALE 0.01

#define FREQS_PER_CHANNEL ((int) (((float) REAL_FFT_SIZE) / 3 / BIN_SHRINKAGE))
#define FREQS_IN_TOP_CHANNEL ((int) ((REAL_FFT_SIZE % NUM_CHANNELS) / BIN_SHRINKAGE))

/* Filtering config */
#define BIN_FILTER_CUTOFF_HZ {4, 4, 5, 6}
#define CHANNEL_GAIN {0.1, 1.4, 1.4, 0.5}
#define BIN_FILTER_SAMPLE_TIME (((float) BUFSIZE) / ((float) AUDIO_SAMPLE_RATE))
#define BIN_FILTER_CONSTANT ((BIN_FILTER_SAMPLE_TIME * 6.283185307179586) / (1 + (BIN_FILTER_SAMPLE_TIME * 6.283185307179586)))

/* channel gain adaptation config */
#define CHAN_GAIN_FILTER_CUTOFF_HZ {0.01, 0.01, 0.01, 0.01}
#define CHAN_GAIN_FILTER_SAMPLE_TIME (((float) BUFSIZE) / ((float) AUDIO_SAMPLE_RATE))
#define CHAN_GAIN_FILTER_CONSTANT ((CHAN_GAIN_FILTER_SAMPLE_TIME * 6.283185307179586) / (1 + (CHAN_GAIN_FILTER_SAMPLE_TIME * 6.283185307179586)))
#define CHAN_GAIN_MAX 0.6
#define CHAN_GAIN_GOAL_ACTIVITY {0.2, 0.4, 0.5, 0.2}
#define CHAN_GAIN_BUMP 0.001
#define CHAN_GAIN_UPDATE_BUMP 0.05

/* Serial I/O config */
#define BAUDRATE B115200
#define LIGHT_SHOW_DEVICE "/dev/ttyUSB0"

#define TTY_CTRL_OPTS (CS8 | CLOCAL | CREAD)
#define TTY_INPUT_OPTS IGNPAR 
#define TTY_OUTPUT_OPTS 0
#define TTY_LOCAL_OPTS 0

#define MAGIC_CODE 0xDEADBEEF

#endif // __CONFIG_H__
