#include "seam_carving/visualize.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

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

cv::Mat drawHorizontalSeam(const cv::Mat& img, const Seam& seam, const cv::Scalar& colour) {
    CV_Assert(img.type() == CV_8UC3);
    CV_Assert(isValidSeam(seam, img.cols, img.rows)); // one row index per column

    cv::Mat out = img.clone();
    const cv::Vec3b pixel(cv::saturate_cast<uchar>(colour[0]),
                          cv::saturate_cast<uchar>(colour[1]),
                          cv::saturate_cast<uchar>(colour[2]));
    for (int c = 0; c < out.cols; ++c) out.at<cv::Vec3b>(seam[c], c) = pixel;
    return out;
}

cv::Mat renderSeamStep(const cv::Mat& img, const Seam& seam, bool horizontal,
                       const std::string& label) {
    CV_Assert(img.type() == CV_8UC3);
    CV_Assert(horizontal ? isValidSeam(seam, img.cols, img.rows)
                         : isValidSeam(seam, img.rows, img.cols));

    const int gap = 16;
    const int header = 48; // two lines: label and key help

    // Same fitting rule as sideBySide: shrink large images, enlarge small
    // ones by a whole number (up to 4x) so their pixels stay sharp.
    double scale = std::min(1200.0 / img.cols, 650.0 / img.rows);
    int interpolation = cv::INTER_AREA;
    if (scale >= 1.0) {
        scale = std::min(std::floor(scale), 4.0);
        interpolation = cv::INTER_NEAREST;
    }
    cv::Mat view;
    cv::resize(img, view,
               cv::Size(std::max(1, static_cast<int>(std::lround(img.cols * scale))),
                        std::max(1, static_cast<int>(std::lround(img.rows * scale)))),
               0, 0, interpolation);

    // Draw the seam after scaling, as a polyline through pixel centres, so a
    // one-pixel seam does not disappear when a large image is shrunk.
    std::vector<cv::Point> points;
    points.reserve(seam.size());
    for (size_t i = 0; i < seam.size(); ++i) {
        const double along = (static_cast<double>(i) + 0.5) * scale;
        const double across = (seam[i] + 0.5) * scale;
        points.emplace_back(static_cast<int>(horizontal ? along : across),
                            static_cast<int>(horizontal ? across : along));
    }
    const int thickness = std::max(2, static_cast<int>(std::lround(scale)));
    cv::polylines(view, points, false, cv::Scalar(0, 0, 255), thickness, cv::LINE_8);

    cv::Mat canvas(header + view.rows + gap, gap + std::max(view.cols, 420) + gap, CV_8UC3,
                   cv::Scalar(40, 40, 40));
    view.copyTo(canvas(cv::Rect(gap, header, view.cols, view.rows)));
    cv::putText(canvas, label, cv::Point(gap, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5,
                cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
    cv::putText(canvas, "Space: pause/resume   N: next seam (paused)   Esc: skip to result",
                cv::Point(gap, 40), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(180, 180, 180), 1,
                cv::LINE_AA);
    return canvas;
}

SeamStepViewer::SeamStepViewer(std::string title, std::string methodName, int totalSeams,
                               int delayMs)
    : title_(std::move(title)), method_(std::move(methodName)), total_(totalSeams),
      delayMs_(std::max(1, delayMs)) {} // waitKey(0) would wait forever, so at least 1 ms

bool SeamStepViewer::operator()(const cv::Mat& img, const Seam& seam, bool horizontal) {
    ++step_;
    const std::string before = std::to_string(img.cols) + "x" + std::to_string(img.rows);
    const std::string after = horizontal
        ? std::to_string(img.cols) + "x" + std::to_string(img.rows - 1)
        : std::to_string(img.cols - 1) + "x" + std::to_string(img.rows);
    const std::string label = method_ + "  |  seam " + std::to_string(step_) + " of " +
                              std::to_string(total_) + "  |  " +
                              (horizontal ? "horizontal" : "vertical") + "  |  " + before +
                              " -> " + after;
    cv::imshow(title_, renderSeamStep(img, seam, horizontal, label));

    for (;;) {
        const int key = cv::waitKey(paused_ ? 50 : delayMs_);
        if (cv::getWindowProperty(title_, cv::WND_PROP_VISIBLE) < 1) return false; // closed
        const int k = (key < 0) ? -1 : (key & 0xFF);
        if (k == 27) {                       // Esc: stop showing, finish carving
            cv::destroyWindow(title_);
            return false;
        }
        if (k == ' ') {                      // Space: toggle pause
            paused_ = !paused_;
            if (!paused_) return true;
            continue;
        }
        if (paused_) {
            if (k == 'n' || k == 'N') return true; // one step while paused
            continue;
        }
        return true;                         // delay passed (or another key): next seam
    }
}

} // namespace sc
