#pragma once
#include <functional>

#include "seam_carving/seam.hpp"

namespace sc {

// Returns a copy of `img` that is one column narrower, with seam[r]
// removed from each row r. Works for any cv::Mat type.
cv::Mat removeVerticalSeam(const cv::Mat& img, const Seam& seam);

// Called by carve() once per seam, just before that seam is removed.
//   img        the image at its current size, always upright (not transposed)
//   seam       the seam about to be removed
//   horizontal false: seam[r] is the column removed from row r
//              true:  seam[c] is the row removed from column c
// Return true to keep receiving seams, false to stop (carving continues
// without further calls, e.g. when the user skips the animation).
using SeamCallback = std::function<bool(const cv::Mat& img, const Seam& seam, bool horizontal)>;

// Shrinks `img` to targetWidth x targetHeight by removing vertical seams,
// then horizontal seams, recomputing the energy map after every removal.
cv::Mat carve(const cv::Mat& img, int targetWidth, int targetHeight, Method method);

// Same, but calls `onSeam` for every seam before removing it (used for the
// step-by-step visualisation). An empty callback behaves like carve() above.
cv::Mat carve(const cv::Mat& img, int targetWidth, int targetHeight, Method method,
              const SeamCallback& onSeam);

} // namespace sc
