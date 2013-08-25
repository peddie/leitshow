/* Copyright 2013 Matthew Peddie <peddie@alum.mit.edu> */
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "./filterbank.h"

/* This whole biquad_cascade setup is swiped from CMSIS, but who the
 * hell wants to try to figure out their (not a real) build system and
 * piles of weird shit? */
typedef struct biquad_cascade {
  float *coeffs;
  float *state;
  uint8_t n_stage;
} biquad_cascade;

static void
biquad_cascade_init(biquad_cascade *b, uint8_t n_stage,
                    float *coeffs, float *state) {
  b->n_stage = n_stage;
  b->coeffs = coeffs;
  b->state = state;
}

static inline float
biquad_cascade_run(biquad_cascade *b, float input) {
  float *pCoeffs = b->coeffs;
  float *pState = b->state;
  float tmp = input;

  uint8_t stage = b->n_stage;
  do {
    /* Reading the coefficients */
    const float b0 = *pCoeffs++;
    const float b1 = *pCoeffs++;
    const float b2 = *pCoeffs++;
    const float a1 = *pCoeffs++;
    const float a2 = *pCoeffs++;

    /*Reading the state values */
    float d1 = pState[0];
    float d2 = pState[1];

    /* Read the input */
    const float Xn = tmp;

    /* y[n] = b0 * x[n] + d1 */
    const float acc0 = (b0 * Xn) + d1;

    /* Store the result in the accumulator in the destination buffer. */
    tmp = acc0;

    /* Every time after the output is computed state should be updated. */
    /* d1 = b1 * x[n] + a1 * y[n] + d2 */
    d1 = ((b1 * Xn) - (a1 * acc0)) + d2;

    /* d2 = b2 * x[n] + a2 * y[n] */
    d2 = (b2 * Xn) - (a2 * acc0);

    /* Store the updated state variables back into the state array */
    *pState++ = d1;
    *pState++ = d2;

    /* decrement the loop counter */
    stage--;
  } while (stage > 0u);
  return tmp;
}


biquad_cascade filters[NUM_FILTERS];

float state[NUM_FILTERS * STATE_SIZE];

/* These were generated with `runghc biquads.hs <NUM_FILTERS>` as a
 * harmonic filter bank (cutoff frequency doubles each time). */
float coeffs[NUM_FILTERS * NUM_COEFFS] = {
  9.371697475264971e-6, 1.8743394950529942e-5, 9.371697475264971e-6, -1.9913225483591699, 0.9913600351490709,  /* 10.7666015625 Hz */
  9.371697475264971e-6, 1.8743394950529942e-5, 9.371697475264971e-6, -1.9913225483591699, 0.9913600351490709,  /* 10.7666015625 Hz */
  3.732519892929843e-5, 7.465039785859686e-5, 3.732519892929843e-5, -1.9826454185041167, 0.9827947192998339,  /* 21.533203125 Hz */
  3.732519892929843e-5, 7.465039785859686e-5, 3.732519892929843e-5, -1.9826454185041167, 0.9827947192998339,  /* 21.533203125 Hz */
  1.4802198653182094e-4, 2.960439730636419e-4, 1.4802198653182094e-4, -1.9652933726226904, 0.9658854605688175,  /* 43.06640625 Hz */
  1.4802198653182094e-4, 2.960439730636419e-4, 1.4802198653182094e-4, -1.9652933726226904, 0.9658854605688175,  /* 43.06640625 Hz */
  5.820761342360482e-4, 1.1641522684720964e-3, 5.820761342360482e-4, -1.9306064272196681, 0.9329347317566122,  /* 86.1328125 Hz */
  5.820761342360482e-4, 1.1641522684720964e-3, 5.820761342360482e-4, -1.9306064272196681, 0.9329347317566122,  /* 86.1328125 Hz */
  2.2515826568466324e-3, 4.503165313693265e-3, 2.2515826568466324e-3, -1.861361146829083, 0.8703674774564693,  /* 172.265625 Hz */
  2.2515826568466324e-3, 4.503165313693265e-3, 2.2515826568466324e-3, -1.861361146829083, 0.8703674774564693,  /* 172.265625 Hz */
  8.442692929079948e-3, 1.6885385858159897e-2, 8.442692929079948e-3, -1.723776172762509, 0.7575469444788289,  /* 344.53125 Hz */
  8.442692929079948e-3, 1.6885385858159897e-2, 8.442692929079948e-3, -1.723776172762509, 0.7575469444788289,  /* 344.53125 Hz */
  2.9954582208092474e-2, 5.990916441618495e-2, 2.9954582208092474e-2, -1.454243586251585, 0.5740619150839548,  /* 689.0625 Hz */
  2.9954582208092474e-2, 5.990916441618495e-2, 2.9954582208092474e-2, -1.454243586251585, 0.5740619150839548,  /* 689.0625 Hz */
  9.763107293781749e-2, 0.19526214587563498, 9.763107293781749e-2, -0.9428090415820632, 0.3333333333333333,  /* 1378.125 Hz */
  9.763107293781749e-2, 0.19526214587563498, 9.763107293781749e-2, -0.9428090415820632, 0.3333333333333333,  /* 1378.125 Hz */
  0.2928932188134524, 0.5857864376269049, 0.2928932188134524, -1.300707181133076e-16, 0.17157287525380988,  /* 2756.25 Hz */
  0.2928932188134524, 0.5857864376269049, 0.2928932188134524, -1.300707181133076e-16, 0.17157287525380988,  /* 2756.25 Hz */
};

void
filter_setup(void) {
  memset(state, 0, sizeof(state));
  for (int i = 0; i < NUM_FILTERS; i++)
    biquad_cascade_init(&filters[i],
                        NUM_BIQUADS,
                        &coeffs[i * NUM_COEFFS],
                        &state[i * STATE_SIZE]);
}

void
filter_step(float input, float output[]) {
  float filter_out;
  filter_out = biquad_cascade_run(&filters[0], input);
  /* Last value in the output sequence is the band output value */
  output[0] = fabsf(filter_out);

  float sub = output[0];

  for (int i = 1; i < NUM_FILTERS; i++) {
    filter_out = biquad_cascade_run(&filters[i], input);
    /* Output is the last value in the filter output sequence minus
     * the accumulated "lower than this band" value. */
    output[i] = filter_out - sub;
    /* Add this output to the "lower than this band" state for the
     * next band. */
    sub += output[i];
    output[i] = fabsf(output[i]);
  }

  /* The top frequency band is just the last input value minus the
   * accumulated power for all the lower bands. */
  output[NUM_BANDS - 1] = fabsf(input - sub);
}
