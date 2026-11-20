#include "seam_carving/greedy.hpp"

namespace sc {

Seam findSeamGreedy(const cv::Mat& energy) {
    // Step 1: validate input. computeEnergy() always gives CV_64FC1.
    CV_Assert(!energy.empty());
    CV_Assert(energy.type() == CV_64FC1);

    const int rows = energy.rows;
    const int cols = energy.cols;
    Seam seam(rows);

    // Step 2: start at the lowest-energy pixel of the top row.
    // Ties go to the leftmost column (strict "<").
    int col = 0;
    for (int c = 1; c < cols; ++c) {
        if (energy.at<double>(0, c) < energy.at<double>(0, col)) {
            col = c;
        }
    }
    seam[0] = col;

    // Step 3: walk down one row at a time. From the current column, move to
    // the cheapest of the (up to) three pixels below: down-left, down,
    // down-right. Only the next row is looked at, never further ahead, and
    // a choice is never revisited. That is what makes it greedy, fast, and
    // not guaranteed to find the minimum seam (compare findSeamDP).
    for (int r = 1; r < rows; ++r) {
        const int prev = seam[r - 1];
        const int lo = (prev > 0) ? prev - 1 : prev;
        const int hi = (prev < cols - 1) ? prev + 1 : prev;
        int bestCol = lo;
        for (int k = lo + 1; k <= hi; ++k) {
            if (energy.at<double>(r, k) < energy.at<double>(r, bestCol)) {
                bestCol = k;
            }
        }
        seam[r] = bestCol;
    }

    // Time O(cols + rows): one scan of the top row, then 3 comparisons per
    // row. Space O(rows) for the seam itself.
    return seam;
}

} // namespace sc
