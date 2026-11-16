#pragma once
#include "seam_carving/seam.hpp"

namespace sc {

// Minimum-energy vertical seam as a minimum s-t cut, solved with max-flow.
Seam findSeamGraphCut(const cv::Mat& energy);

} // namespace sc
