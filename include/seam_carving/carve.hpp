#pragma once
#include "seam_carving/seam.hpp"

namespace sc {

// Returns a copy of `img` that is one column narrower, with seam[r]
// removed from each row r. Works for any cv::Mat type.
cv::Mat removeVerticalSeam(const cv::Mat& img, const Seam& seam);

// Shrinks `img` to targetWidth x targetHeight by removing vertical seams,
// then horizontal seams, recomputing the energy map after every removal.
cv::Mat carve(const cv::Mat& img, int targetWidth, int targetHeight, Method method);

} // namespace sc
