#include "seam_carving/energy.hpp"

namespace sc {

cv::Mat computeEnergy(const cv::Mat& img) {
    // TODO: implement the energy map (assignment item 1a).
    // Suggested: convert to grayscale, take Sobel gradients in x and y
    // (cv::Sobel), and return |gx| + |gy| as CV_64FC1.
    //
    // Placeholder: every pixel has zero energy.
    return cv::Mat::zeros(img.rows, img.cols, CV_64FC1);
}

} // namespace sc
