#include <math.h>
#include <string.h>

#include <fftw3.h>

#include "config.h"

#include "newadap.h"

#include "arraymath.h"

#define IND_MIN(a, b) ((a) > (b)) ? (b) : (a)
#define IND_MAX(a, b) ((a) < (b)) ? (b) : (a)

static float correlations[NUM_BOUNDS];

static unsigned int bin_update;

/* Compute one update to an IIR estimate of the correlation between
 * two quantities.  This is maybe less accurate than a window, but
 * it's cheap! */
static inline float
iir_correlation(float a, float b, float corr_1)
{
  return a*b * CORR_FILTER_CONSTANT + (1 - CORR_FILTER_CONSTANT) * corr_1;
}

static inline float
iir_complex_correlation(fftwf_complex a, fftwf_complex b, float corr_1)
{
  /* Compute a normalized correlation to decouple this from power
   * optimization. */
  float real = a[0] * b[0];
  float imag = a[1] * b[1];
  float normalize = sqrt(a[0]*a[0] + a[1]*a[1]
                         + b[0]*b[0] + b[1]*b[1]);
  return (real + imag) / fmaxf(normalize, 1e-5) * CORR_FILTER_CONSTANT
      + (1 - CORR_FILTER_CONSTANT) * corr_1;
}

/* Compute the global cost for a bin boundary configuration. */
static inline float
bin_bound_cost(float bin_powers[NUM_CHANNELS],
               int bin_bounds[NUM_BOUNDS],
               float this_corr)
{
  /* Penalize high variance -- we want to equalize power between
   * channels. */
  float variance = CORR_VARIANCE_WEIGHT * array_variance(bin_powers, NUM_CHANNELS);

  /* Penalize high correlations between channels -- we want to map
   * channels to parts of the song that behave differently in time. */
  float corr = CORR_XCORR_WEIGHT * this_corr;

  /* Penalize inactive bins -- we want to keep all the channels
   * busy! */
  const float min_ok_power = 0.5;
  float min = CORR_MIN_WEIGHT
      * (fminf(min_ok_power - array_min(bin_powers, NUM_CHANNELS),
               min_ok_power));

  /* Penalize really narrow bins -- we want to keep all the channels
   * busy! */
  const int min_ok_size = 25;

  /* Compute the bin sizes */
  int bin_sizes[NUM_CHANNELS];
  bin_sizes[0] = bin_bounds[0];
  for (int i = 1; i < NUM_CHANNELS - 1; i++)
    bin_sizes[i] = bin_bounds[i] - bin_bounds[i-1];
  
  bin_sizes[NUM_CHANNELS - 1] = REAL_FFT_SIZE - bin_bounds[NUM_BOUNDS - 1];

  int minsize = array_int_min(bin_sizes, NUM_CHANNELS);
  float narrow = CORR_NARROW_WEIGHT
      * (minsize < min_ok_size) * (min_ok_size - minsize);
  
  fprintf(stdout, "\tcost evaluation: %f %f %f %f\n",
          variance, corr, min, narrow);
  
  return variance + corr + min + narrow;
}

void
adap_decorr_init(void)
{
  int i;
  for (i = 0; i < NUM_BOUNDS; i++)
    correlations[i] = 0;
  bin_update = 1;
}

/* Compute new bin boundaries so as to optimize the cost function
 * bin_bound_cost() for each bin boundary.  The optimization is
 * performed in a SISO fashion and scales linearly with NUM_CHANNELS *
 * CORR_OPT_RANGE.  A MIMO optimization would scale quadratically.
 */
void
bin_bound_adjust(int bin_bounds[NUM_BOUNDS],
                 const float bin_powers[NUM_CHANNELS],
                 const float freq_powers[REAL_FFT_SIZE],
                 const fftwf_complex freq_data[REAL_FFT_SIZE])
{

  int old_bin_bounds[NUM_BOUNDS];
  memcpy(old_bin_bounds, bin_bounds, sizeof(old_bin_bounds));
  /* Loop over bin boundaries and compute the optimal update for
   * each.  */
  for (int i = 0; i < NUM_BOUNDS; i++) {
    if (bin_update % CORR_UPDATE_EVERY == 0) {
      /* Compute safe indices within the FFT data within which to search
       * to optimize this bin boundary. */
      int safe_search_min, safe_search_max;
      /* Compute indices within the FFT data over which to sum to get
       * the neighboring bin powers. */
      int bin_below_min, bin_above_max;
      if (i == 0) {
        safe_search_min = IND_MAX(0, bin_bounds[i] - CORR_OPT_RANGE - 1);
        bin_below_min = 0;
      } else {
        safe_search_min = IND_MAX(bin_bounds[i-1] + 1,
                                  bin_bounds[i] - CORR_OPT_RANGE - 1);
        bin_below_min = bin_bounds[i - 1];
      }
      if (i == NUM_BOUNDS - 1) {
        safe_search_max = IND_MIN(REAL_FFT_SIZE, bin_bounds[i] + CORR_OPT_RANGE);
        bin_above_max = REAL_FFT_SIZE;
      } else {
        safe_search_max = IND_MIN(bin_bounds[i+1] - 1,
                                  bin_bounds[i] + CORR_OPT_RANGE);
        bin_above_max= bin_bounds[i+1] - 1;
      }

      int safe_range = safe_search_max - safe_search_min;
      
      float costs[safe_range];
      float corrs[safe_range];
      float this_pow[NUM_CHANNELS];
      memcpy(this_pow, bin_powers, sizeof(this_pow));
      /* Test a range of new bin boundaries around the current one.  j
       * is the index within the FFT data; h is the index within the
       * costs and corrs arrays. */
      for (int j = safe_search_min, h = 0; j < safe_search_max; j++, h++) {
        /* Compute dot products (real and complex) for the channels
         * below and above this one. */
        float below = array_mean(&freq_powers[bin_below_min], j - bin_below_min);
        float above = array_mean(&freq_powers[j], bin_above_max - j);

        fftwf_complex below_cx, above_cx;
        array_complex_mean(&freq_data[bin_below_min], j - bin_below_min, &below_cx);
        array_complex_mean(&freq_data[j], bin_above_max - j, &above_cx);

        /* Compute the correlation for this pair. */
        corrs[h] = iir_complex_correlation(below_cx, above_cx, correlations[i]);

        /* Compute the cost for this pair. */
        this_pow[i] = below;
        this_pow[i+1] = above;
        costs[h] = bin_bound_cost(this_pow, old_bin_bounds, corrs[h]);
#define SERIOUS_DEBUG_ADAP        
#ifdef SERIOUS_DEBUG_ADAP        
        printf("    ADAP: bound %d: search %d of %d: %d -> %d  xcorr %f"
               "neighbors (%f, %f) (%f, %f)\n",
               i, h, safe_range, bin_bounds[i], safe_search_min + h, corrs[h],
               below_cx[0], below_cx[1], above_cx[0], above_cx[1]);
#endif  /* SERIOUS_DEBUG_ADAP */
      }

      /* Choose the best new bin bound (the minimum of the cost function). */
      int best = array_min_ind(costs, safe_range);
#define DEBUG_ADAP
      
      /* Update the bin bound and correlation */
#ifdef DEBUG_ADAP
      int old = bin_bounds[i];
#endif  /* DEBUG_ADAP */
      bin_bounds[i] = safe_search_min + best;
      correlations[i] = corrs[best];

      /* Debugging message */
#ifdef DEBUG_ADAP
      printf("  ADAP: bound %d: %d -> %d   xcorr %f  best %d (%f) of (",
             i, old, bin_bounds[i], correlations[i], best, costs[best]);

      for (int j = 0; j < safe_range; j++)
        printf("%f ", costs[j]);
      printf(")\n");
      
#endif  /* DEBUG_ADAP */
    } else {
      /* Just update the correlation with the existing bins; we didn't
       * change bin boundaries. */
      int bin_below_min, bin_above_max;
      if (i == 0)
        bin_below_min = 0;
      else
        bin_below_min = bin_bounds[i - 1];
      if (i == NUM_BOUNDS - 1)
        bin_above_max = REAL_FFT_SIZE;
      else
        bin_above_max= bin_bounds[i+1] - 1;

      fftwf_complex below_cx, above_cx;
      array_complex_mean(&freq_data[bin_below_min],
                         bin_bounds[i] - bin_below_min, &below_cx);
      array_complex_mean(&freq_data[bin_bounds[i]],
                         bin_above_max - bin_bounds[i], &above_cx);

      correlations[i] = iir_complex_correlation(below_cx, above_cx, correlations[i]);
    }
/*     printf("%f ", correlations[i]); */
  }
/*   printf("\t"); */
  bin_update++;
}
