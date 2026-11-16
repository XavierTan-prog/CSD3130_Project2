#pragma once
#include "seam_carving/seam.hpp"

namespace sc {

// Vertical seam chosen row by row, taking the locally cheapest neighbour.
Seam findSeamGreedy(const cv::Mat& energy);

} // namespace sc
