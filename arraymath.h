#include <fftw3.h>

#ifndef __ARRAYMATH_H__
#define __ARRAYMATH_H__

static inline void
array_scale(float arr[], float scale, int n)
{
  for (int i = 0; i < n; i++)
    arr[i] *= scale;
}

static inline float
array_mean(const float arr[], int n)
{
  float mean = 0;
  for (int i = 0; i < n; i++)
    mean += arr[i];
  return fmaxf(mean / n, 1e-5);
}

static inline void
array_complex_mean(const fftwf_complex arr[], int n, fftwf_complex *mean)
{
  *mean[0] = *mean[1] = 0;
  for (int i = 0; i < n; i++) {
    *mean[0] += arr[i][0];
    *mean[1] += arr[i][1];
  }
  *mean[0] = fmaxf(*mean[0] / n, 1e-5);
  *mean[1] = fmaxf(*mean[1] / n, 1e-5);
}

static inline float
array_variance(const float arr[], int n)
{
  float variance = 0, mean = array_mean(arr, n);
  for (int i = 0; i < n; i++)
    variance += (arr[i] - mean) * (arr[i] - mean);
  return fmaxf(variance / n, 1e-5);
}

static inline float
array_min(const float costs[], int n)
{
  float min = 22e10;
  for (int i = 0; i < n; i++)
    if (costs[i] < min)
      min = costs[i];
  return min;
}

static inline int
array_min_ind(const float costs[], int n)
{
  float min = 1e22;
  int minind = 0;
  for (int i = 0; i < n; i++)
    if (costs[i] < min) {
      minind = i;
      min = costs[i];
    }
  return minind;
}

static inline int
array_max_ind(const float costs[], int n)
{
  float max = -1e22;
  int maxind = 0;
  for (int i = 0; i < n; i++)
    if (costs[i] > max) {
      maxind = i;
      max = costs[i];
    }
  return maxind;
}

static inline int
array_int_min(const int costs[], int n)
{
  int min = 22222222;
  for (int i = 0; i < n; i++)
    if (costs[i] < min)
      min = costs[i];
  return min;
}

#endif  /* __ARRAYMATH_H__ */
