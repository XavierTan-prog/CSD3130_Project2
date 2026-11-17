#include "seam_carving/energy.hpp"

#include <opencv2/imgproc.hpp>

namespace sc {

cv::Mat computeEnergy(const cv::Mat& img, EnergyOperator op) {
    // Step 1: validate input. The application always passes BGR 8-bit
    // images; anything else is a caller bug, so fail loudly.
    CV_Assert(!img.empty());
    CV_Assert(img.type() == CV_8UC3);

    // Step 2: work on luminance only. A single gray channel captures the
    // visible edges and keeps the gradient cost to one channel.
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);

    // Step 3: 3x3 Sobel derivatives in x and y. The output depth is CV_64F
    // so negative gradients are kept (CV_8U would clip them to 0).
    // BORDER_REFLECT_101 mirrors pixels without repeating the edge pixel,
    // so a flat image has zero energy on its border too.
    cv::Mat gradientX;
    cv::Mat gradientY;
    cv::Sobel(gray, gradientX, CV_64F, 1, 0, 3, 1.0, 0.0, cv::BORDER_REFLECT_101);
    cv::Sobel(gray, gradientY, CV_64F, 0, 1, 3, 1.0, 0.0, cv::BORDER_REFLECT_101);

    // Step 4: combine the two derivatives into one magnitude per pixel.
    // Only the selected operator is computed.
    cv::Mat energy;
    switch (op) {
        case EnergyOperator::SobelL1:
            // |gx| + |gy|, computed in double precision.
            energy = cv::abs(gradientX) + cv::abs(gradientY);
            break;
        case EnergyOperator::SobelL2:
            // sqrt(gx^2 + gy^2) via OpenCV's element-wise magnitude.
            cv::magnitude(gradientX, gradientY, energy);
            break;
        default:
            CV_Error(cv::Error::StsBadArg, "computeEnergy: unknown EnergyOperator");
    }

    // Step 5: postcondition the seam finders rely on: one double per pixel,
    // same size as the input. Values are left raw (not normalized).
    CV_Assert(energy.type() == CV_64FC1);
    CV_Assert(energy.rows == img.rows && energy.cols == img.cols);
    return energy;
}

} // namespace sc
