#pragma once
#include "seam_carving/seam.hpp"

namespace sc {

// Minimum-energy vertical seam by dynamic programming (globally optimal).
Seam findSeamDP(const cv::Mat& energy);

} // namespace sc
