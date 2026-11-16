#pragma once
#include <opencv2/core.hpp>

namespace sc {

// Energy (disruption measure) of every pixel.
// Input:  BGR image, CV_8UC3.
// Output: CV_64FC1, same size as the input.
cv::Mat computeEnergy(const cv::Mat& img);

} // namespace sc
