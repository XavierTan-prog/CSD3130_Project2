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

// Like drawSeam, for a horizontal seam: seam[c] is the row painted in column c.
cv::Mat drawHorizontalSeam(const cv::Mat& img, const Seam& seam,
                           const cv::Scalar& colour = cv::Scalar(0, 0, 255));

// Scale used to fit a cols x rows image on screen in the step-by-step view:
// large images are shrunk, small ones enlarged by a whole number (up to 4x).
double stepViewScale(int cols, int rows);

// One frame of the step-by-step view: `img` (CV_8UC3) scaled to fit on
// screen, the seam drawn over it as a red line that stays visible after
// scaling, and `label` plus the key help above it.
// `scale` > 0 fixes the zoom instead of fitting this image, and `frame`
// (cols x rows of the image area, before scaling) fixes the canvas size, so
// every frame of an animation has the same zoom and window size and the
// picture visibly gets narrower / shorter inside the frame.
cv::Mat renderSeamStep(const cv::Mat& img, const Seam& seam, bool horizontal,
                       const std::string& label, double scale = 0.0,
                       cv::Size frame = cv::Size());

// Step-by-step visualisation (bonus item): pass it to carve() as the seam
// callback (wrapped in std::ref) and it shows every seam before it is
// removed. Keys in the window: Space pauses/resumes, N steps one seam while
// paused, Esc skips the rest of the animation (carving still finishes).
class SeamStepViewer {
public:
    // `target` is the final size, shown in the label (optional).
    SeamStepViewer(std::string title, std::string methodName, int totalSeams, int delayMs,
                   cv::Size target = cv::Size());

    // Matches sc::SeamCallback. Returns false once the user skips or closes the window.
    bool operator()(const cv::Mat& img, const Seam& seam, bool horizontal);

    int stepsShown() const { return step_; }

private:
    std::string title_;
    std::string method_;
    int total_;
    int delayMs_;
    cv::Size target_;
    cv::Size start_;      // size of the image at the first seam
    double scale_ = 0.0;  // zoom fixed at the first seam
    int step_ = 0;
    bool paused_ = false;
};

} // namespace sc
