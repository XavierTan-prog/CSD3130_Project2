// Results harness: the 5 x 5 worked example from the report.
// Expected: DP cost 8, greedy 23, graph cut 8.
#include <cstdio>

#include "seam_carving/dp.hpp"
#include "seam_carving/graph_cut.hpp"
#include "seam_carving/greedy.hpp"
#include "seam_carving/seam.hpp"

using namespace sc;

int main()
{
    double data[25] = {
        5, 1, 6, 4, 2,
        6, 2, 7, 3, 1,
        9, 8, 9, 2, 3,
        7, 9, 8, 1, 4,
        5, 9, 9, 2, 6,
    };
    cv::Mat energy(5, 5, CV_64F, data);

    const char* names[3] = {"dp", "greedy", "graphcut"};
    Seam seams[3] = {findSeamDP(energy), findSeamGreedy(energy), findSeamGraphCut(energy)};

    for (int i = 0; i < 3; ++i) {
        std::printf("%s cost %.0f cols:", names[i], seamCost(energy, seams[i]));
        for (int c : seams[i])
            std::printf(" %d", c);
        std::printf("\n");
    }
    return 0;
}
