#pragma once
#include <string>

#include "seam_carving/seam.hpp"

namespace sc {

// Returns a copy of `img` (CV_8UC3) with the seam's pixels painted in
// `colour` (BGR, red by default). Useful for showing which seam a method chose.
cv::Mat drawSeam(const cv::Mat& img, const Seam& seam,
                 const cv::Scalar& colour = cv::Scalar(0, 0, 255));

// Builds one image with `left` and `right` (both CV_8UC3) next to each
// other on a dark background, each with a text label above it. The pair
// is scaled (large images shrunk, small ones enlarged) to fit on screen.
cv::Mat sideBySide(const cv::Mat& left, const std::string& leftLabel,
                   const cv::Mat& right, const std::string& rightLabel);

// Opens a window showing `image` and waits until a key is pressed or the
// window is closed.
void showWindow(const std::string& title, const cv::Mat& image);

} // namespace sc
