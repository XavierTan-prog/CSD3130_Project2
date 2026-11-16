#include "seam_carving/graph_cut.hpp"

namespace sc {

Seam findSeamGraphCut(const cv::Mat& energy) {
    // TODO: implement (assignment item 4).
    //  1. One node per pixel, plus source and sink.
    //  2. Source -> every pixel in the first column (infinite capacity);
    //     every pixel in the last column -> sink (infinite capacity).
    //  3. (i, j) -> (i, j+1) with capacity e[i][j].
    //  4. Infinite backward edges (i, j+1) -> (i, j) and diagonal edges
    //     (i, j) -> (i-1, j-1), (i, j) -> (i+1, j-1).
    //  5. Run max-flow (Edmonds-Karp), BFS from the source in the residual
    //     graph; the rightmost reachable pixel in each row is the seam.
    //
    // Placeholder: a straight seam down column 0.
    return Seam(energy.rows, 0);
}

} // namespace sc
