#include <stdio.h>

#include <iostream>
#include <chrono>

#include <Eigen/Core>
#include <Eigen/SVD>
#include <Eigen/Eigenvalues>

#include "./config.h"

#include "./pca.h"

// Eigen::MatrixXd eigenvectors_of_covariance(const Eigen::MatrixXd &P) {
//   P.const Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> decomp(P);
//   return decomp.eigenvectors();
// }

Eigen::MatrixXd pc_vectors_of_samples(const Eigen::MatrixXd &x) {
  // One big mondo expression so Eigen can have its way with it.
  return (x.rowwise() - x.colwise().mean())
      .jacobiSvd(Eigen::ComputeThinV)
      .matrixV();
}

void SampleBuffer::update(const Eigen::VectorXd &bins) {
  history.row(history_idx++) = bins;
  history_idx = history_idx % history.rows();
}

Eigen::VectorXd activate_samples_eigen(const Eigen::VectorXd &bins,
                                       SampleBuffer *history) {
  // Step 1: compute the eigenvectors of the time history's covariance
  const Eigen::MatrixXd V = pc_vectors_of_samples(history->history);

  // Step 2: compute the activation level for the current timestep
  const Eigen::VectorXd bin_means(history->history.colwise().mean());
  const Eigen::VectorXd activation = V.transpose() * (bins - bin_means);

  // Step 3: update the time history with the current timestep
  history->update(bins);

  // for (int i = 0; i < activation.size(); i++) {
  //   fprintf(stderr, "%.2e ", activation(i));
  // }
  // fprintf(stderr, "    ");

  return activation;
}

void activate_samples(std::size_t bin_count, const float bins[],
                      SampleBuffer *samples, float activations[]) {
  const Eigen::Map<const Eigen::VectorXf> binvec(bins, bin_count);
  Eigen::Map<Eigen::VectorXf> actvec(activations, bin_count);

  actvec = activate_samples_eigen(binvec.cast<double>(), samples).cast<float>();
}

SampleBuffer *new_sample_buffer(size_t buflen, size_t dimension) {
  return new SampleBuffer(buflen, dimension);
}