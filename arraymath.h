#ifndef __ARRAYMATH_H__
#define __ARRAYMATH_H__

static inline void
array_scale(float arr[], float scale, int n)
{
  for (int i = 0; i < n; i++)
    arr[i] *= scale;
}

static inline float
array_sum(float arr[], int n) {
  float sum = 0;
  for (int i = 0; i < n; i++) {
    sum += arr[i];
  }
  return sum;
}

static inline float
array_mean(float arr[], int n)
{
  float mean = 0;
  for (int i = 0; i < n; i++)
    mean += arr[i];
  return mean / n;
}

static inline float
array_variance(float arr[], int n)
{
  float variance = 0, mean = array_mean(arr, n);
  for (int i = 0; i < n; i++)
    variance += (arr[i] - mean) * (arr[i] - mean);
  return variance / n;
}

static inline int
array_max_ind(float costs[], int n)
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

#endif  /* __ARRAYMATH_H__ */
