#pragma once
#include <opencv2/core.hpp>

namespace sc {

/// How the horizontal and vertical Sobel derivatives are combined into one
/// energy value per pixel.
enum class EnergyOperator {
    SobelL1, ///< L1 magnitude: |gx| + |gy|. Default; cheap and standard for seam carving.
    SobelL2  ///< L2 (Euclidean) magnitude: sqrt(gx^2 + gy^2).
};

/**
 * @brief Computes the per-pixel energy (disruption) map of an image from the
 *        Sobel gradient of its grayscale version. High values mark edges and
 *        detail that would be visibly disrupted if the pixel were removed.
 *
 * @param img BGR input image of type CV_8UC3, non-empty. It is not modified.
 * @param op  How gx and gy are combined: EnergyOperator::SobelL1 (|gx| + |gy|,
 *            the default) or EnergyOperator::SobelL2 (sqrt(gx^2 + gy^2)).
 * @return Raw, non-normalized energy map of type CV_64FC1 with exactly the
 *         same rows and columns as @p img. All values are >= 0.
 *
 * @throws cv::Exception if @p img is empty or not CV_8UC3, or if @p op is not
 *         a valid EnergyOperator value.
 */
cv::Mat computeEnergy(const cv::Mat& img, EnergyOperator op = EnergyOperator::SobelL1);

} // namespace sc
