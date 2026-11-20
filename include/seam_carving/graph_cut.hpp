#pragma once
#include "seam_carving/seam.hpp"

namespace sc {

// Minimum-energy vertical seam as a minimum s-t cut, solved with max-flow.
// Expects a non-empty, two-dimensional CV_64FC1 energy map with finite,
// nonnegative values. Invalid maps or residual overflow throw cv::Exception.
Seam findSeamGraphCut(const cv::Mat& energy);

} // namespace sc
