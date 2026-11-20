#include "seam_carving/carve.hpp"

#include <cstring>

#include "seam_carving/energy.hpp"

namespace sc {

cv::Mat removeVerticalSeam(const cv::Mat& img, const Seam& seam) {
    CV_Assert(img.cols > 1);
    CV_Assert(isValidSeam(seam, img.rows, img.cols));

    cv::Mat out(img.rows, img.cols - 1, img.type());
    const size_t px = img.elemSize(); // bytes per pixel
    for (int r = 0; r < img.rows; ++r) {
        const uchar* src = img.ptr(r);
        uchar* dst = out.ptr(r);
        const size_t c = static_cast<size_t>(seam[r]);
        const size_t after = static_cast<size_t>(img.cols) - c - 1;
        std::memcpy(dst, src, c * px);                              // left of the seam
        std::memcpy(dst + c * px, src + (c + 1) * px, after * px);  // right of the seam
    }
    return out;
}

// Removes vertical seams until `img` is `targetCols` wide. When
// `transposed` is true, `img` is the transposed picture (so these are
// really horizontal seams); the callback is then given the upright image.
// `notify` is cleared once the callback asks to stop.
static cv::Mat shrinkWidth(cv::Mat img, int targetCols, Method method, bool transposed,
                           const SeamCallback& onSeam, bool& notify) {
    while (img.cols > targetCols) {
        const cv::Mat energy = computeEnergy(img);
        const Seam seam = findSeam(energy, method);
        if (notify && onSeam) {
            const cv::Mat upright = transposed ? cv::Mat(img.t()) : img;
            notify = onSeam(upright, seam, transposed);
        }
        img = removeVerticalSeam(img, seam);
    }
    return img;
}

cv::Mat carve(const cv::Mat& input, int targetWidth, int targetHeight, Method method) {
    return carve(input, targetWidth, targetHeight, method, SeamCallback{});
}

cv::Mat carve(const cv::Mat& input, int targetWidth, int targetHeight, Method method,
              const SeamCallback& onSeam) {
    CV_Assert(targetWidth >= 1 && targetWidth <= input.cols);
    CV_Assert(targetHeight >= 1 && targetHeight <= input.rows);

    bool notify = static_cast<bool>(onSeam);
    cv::Mat img = shrinkWidth(input.clone(), targetWidth, method, false, onSeam, notify);

    // A horizontal seam is a vertical seam of the transposed image.
    if (img.rows > targetHeight) {
        cv::Mat transposed = img.t();
        transposed = shrinkWidth(transposed, targetHeight, method, true, onSeam, notify);
        img = transposed.t();
    }
    return img;
}

} // namespace sc
