#include "seam_carving/greedy.hpp"

namespace sc {

Seam findSeamGreedy(const cv::Mat& energy) {
    // TODO: implement (assignment item 3).
    // Start at the minimum-energy pixel of the top row, then in each
    // following row move to the cheapest of the (up to) three pixels below.
    //
    // Placeholder: a straight seam down column 0.
    return Seam(energy.rows, 0);
}

} // namespace sc
