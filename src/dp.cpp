#include "seam_carving/dp.hpp"

namespace sc {

Seam findSeamDP(const cv::Mat& energy) {
    // TODO: implement (assignment item 1b).
    //   M[i][j] = e[i][j] + min(M[i-1][j-1], M[i-1][j], M[i-1][j+1])
    // Fill M top to bottom, take the minimum of the last row, then
    // backtrack upwards to recover the seam.
    //
    // Placeholder: a straight seam down column 0.
    return Seam(energy.rows, 0);
}

} // namespace sc
