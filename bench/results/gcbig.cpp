// Results harness: one graph-cut seam on the two largest image sizes, with
// its seam energy next to DP's. Takes about 35 s. See run_results.bat.
#include <chrono>
#include <cmath>
#include <cstdio>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "seam_carving/dp.hpp"
#include "seam_carving/energy.hpp"
#include "seam_carving/graph_cut.hpp"
#include "seam_carving/paths.hpp"
#include "seam_carving/seam.hpp"

using namespace sc;
using Clock = std::chrono::steady_clock;

int main()
{
    useProjectDir();
    cv::Mat tower = cv::imread("images/input/Broadway_tower_edit.jpg");
    if (tower.empty()) {
        std::printf("Could not read images/input. Run from the project folder.\n");
        return 1;
    }

    for (int w : {714, 1428}) {
        int h = static_cast<int>(std::lround(tower.rows * static_cast<double>(w) / tower.cols));
        cv::Mat im;
        if (w == tower.cols)
            im = tower;
        else
            cv::resize(tower, im, cv::Size(w, h), 0, 0, cv::INTER_AREA);

        cv::Mat e = computeEnergy(im);
        auto start = Clock::now();
        Seam seam = findSeamGraphCut(e);
        double ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();

        std::printf("%dx%d graphcut one seam %.0f ms, cost %.1f (DP %.1f)\n",
                    w, h, ms, seamCost(e, seam), seamCost(e, findSeamDP(e)));
        std::fflush(stdout);
    }
    return 0;
}
