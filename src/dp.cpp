#include "seam_carving/dp.hpp"

#include <vector>

namespace sc {

Seam findSeamDP(const cv::Mat& energy) {
    // Step 1: validate input. computeEnergy() always gives CV_64FC1.
    CV_Assert(!energy.empty());
    CV_Assert(energy.type() == CV_64FC1);

    const int rows = energy.rows;
    const int cols = energy.cols;

    // Step 2: the DP table. M[r][c] is the minimum total energy of any
    // connected seam that starts somewhere in row 0 and ends at (r, c).
    // Stored as one flat vector, indexed r * cols + c.
    std::vector<double> M(static_cast<size_t>(rows) * cols);
    auto at = [&](int r, int c) -> double& { return M[static_cast<size_t>(r) * cols + c]; };

    // Base case: a seam ending in row 0 is just that one pixel.
    for (int c = 0; c < cols; ++c) {
        at(0, c) = energy.at<double>(0, c);
    }

    // Step 3: fill the table top to bottom with the recurrence
    //   M[r][c] = e[r][c] + min(M[r-1][c-1], M[r-1][c], M[r-1][c+1])
    // Neighbours outside the image are skipped (clamped column range).
    for (int r = 1; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const int lo = (c > 0) ? c - 1 : c;
            const int hi = (c < cols - 1) ? c + 1 : c;
            double best = at(r - 1, lo);
            for (int k = lo + 1; k <= hi; ++k) {
                if (at(r - 1, k) < best) {
                    best = at(r - 1, k);
                }
            }
            at(r, c) = energy.at<double>(r, c) + best;
        }
    }

    // Step 4: the optimal seam ends at the smallest value in the last row.
    // Ties go to the leftmost column (strict "<"), so results are deterministic.
    Seam seam(rows);
    int col = 0;
    for (int c = 1; c < cols; ++c) {
        if (at(rows - 1, c) < at(rows - 1, col)) {
            col = c;
        }
    }
    seam[rows - 1] = col;

    // Step 5: backtrack upwards. In each row above, move to whichever of the
    // (up to) three neighbours holds the minimum, i.e. the cell the
    // recurrence took its value from. Ties again go to the leftmost.
    for (int r = rows - 2; r >= 0; --r) {
        const int prev = seam[r + 1];
        const int lo = (prev > 0) ? prev - 1 : prev;
        const int hi = (prev < cols - 1) ? prev + 1 : prev;
        int bestCol = lo;
        for (int k = lo + 1; k <= hi; ++k) {
            if (at(r, k) < at(r, bestCol)) {
                bestCol = k;
            }
        }
        seam[r] = bestCol;
    }

    // Time O(rows * cols): every cell is filled once with 3 comparisons,
    // and backtracking is O(rows). Space O(rows * cols) for the table.
    return seam;
}

} // namespace sc
