#include "seam_carving/visualize.hpp"

#include <algorithm>
#include <cmath>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

namespace sc {

cv::Mat drawSeam(const cv::Mat& img, const Seam& seam, const cv::Scalar& colour) {
    CV_Assert(img.type() == CV_8UC3);
    CV_Assert(isValidSeam(seam, img.rows, img.cols));

    cv::Mat out = img.clone();
    const cv::Vec3b pixel(cv::saturate_cast<uchar>(colour[0]),
                          cv::saturate_cast<uchar>(colour[1]),
                          cv::saturate_cast<uchar>(colour[2]));
    for (int r = 0; r < out.rows; ++r) out.at<cv::Vec3b>(r, seam[r]) = pixel;
    return out;
}

cv::Mat sideBySide(const cv::Mat& left, const std::string& leftLabel,
                   const cv::Mat& right, const std::string& rightLabel) {
    CV_Assert(left.type() == CV_8UC3 && right.type() == CV_8UC3);

    const int gap = 16;    // space around and between the images
    const int header = 28; // room for the labels

    // Fit both images into roughly 1200 x 600 pixels so the window fits on
    // screen. Large images are shrunk; small ones are enlarged by a whole
    // number (up to 4x) so their pixels stay sharp. Both use the same scale.
    const double maxWidth = 1200.0;
    const double maxHeight = 600.0;
    double scale = std::min(maxWidth / (left.cols + right.cols),
                            maxHeight / std::max(left.rows, right.rows));
    int interpolation = cv::INTER_AREA;
    if (scale >= 1.0) {
        scale = std::min(std::floor(scale), 4.0);
        interpolation = cv::INTER_NEAREST;
    }
    const auto scaled = [&](const cv::Mat& img) {
        const cv::Size size(std::max(1, static_cast<int>(std::lround(img.cols * scale))),
                            std::max(1, static_cast<int>(std::lround(img.rows * scale))));
        cv::Mat out;
        cv::resize(img, out, size, 0, 0, interpolation);
        return out;
    };
    const cv::Mat l = scaled(left);
    const cv::Mat r = scaled(right);

    const int width = gap + l.cols + gap + r.cols + gap;
    const int height = header + std::max(l.rows, r.rows) + gap;
    cv::Mat canvas(height, width, CV_8UC3, cv::Scalar(40, 40, 40));

    const int rightX = gap + l.cols + gap;
    l.copyTo(canvas(cv::Rect(gap, header, l.cols, l.rows)));
    r.copyTo(canvas(cv::Rect(rightX, header, r.cols, r.rows)));

    const cv::Scalar white(255, 255, 255);
    cv::putText(canvas, leftLabel, cv::Point(gap, header - 9),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, white, 1, cv::LINE_AA);
    cv::putText(canvas, rightLabel, cv::Point(rightX, header - 9),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, white, 1, cv::LINE_AA);
    return canvas;
}

void showWindow(const std::string& title, const cv::Mat& image) {
    cv::imshow(title, image);
    // Poll so that closing the window with the X button also returns.
    while (cv::waitKey(50) < 0) {
        if (cv::getWindowProperty(title, cv::WND_PROP_VISIBLE) < 1) break;
    }
    cv::destroyAllWindows();
}

} // namespace sc
