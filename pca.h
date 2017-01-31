#ifdef __cplusplus
#include <Eigen/Core>

struct SampleBuffer {
  SampleBuffer(std::size_t buflen, std::size_t dimension)
      : history(Eigen::MatrixXd::Zero(buflen, dimension)), history_idx(0) {}

  void update(const Eigen::VectorXd &bins);

  Eigen::MatrixXd history;
  std::size_t history_idx;
};

extern "C" {
#else  // __cplusplus
typedef struct SampleBuffer SampleBuffer;
#endif  // __cplusplus

SampleBuffer *new_sample_buffer(size_t buflen, size_t dimension);

void activate_samples(size_t bin_count, const float bins[],
                      SampleBuffer *samples, float activations[]);

#ifdef __cplusplus
}
#endif  // __cplusplus