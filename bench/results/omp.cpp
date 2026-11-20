// Results harness: our serial DP against a row-parallel OpenMP DP on the full
// tower image. Experiment only; the OpenMP version is not in our source code.
#include <chrono>
#include <cstdio>
#include <vector>

#include <omp.h>
#include <opencv2/imgcodecs.hpp>

#include "seam_carving/dp.hpp"
#include "seam_carving/energy.hpp"
#include "seam_carving/paths.hpp"
#include "seam_carving/seam.hpp"

using namespace sc;
using Clock = std::chrono::steady_clock;

// Full-table DP; the cells of each row are shared among the OpenMP threads.
static Seam dpOmp(const cv::Mat& e)
{
    const int R = e.rows, C = e.cols;
    std::vector<double> M(static_cast<size_t>(R) * C);

    for (int c = 0; c < C; ++c)
        M[c] = e.at<double>(0, c);

    #pragma omp parallel
    for (int r = 1; r < R; ++r) {
        const double* er = e.ptr<double>(r);
        const double* above = &M[static_cast<size_t>(r - 1) * C];
        double* row = &M[static_cast<size_t>(r) * C];

        // The implicit barrier at the end of "omp for" keeps rows in order.
        #pragma omp for schedule(static)
        for (int c = 0; c < C; ++c) {
            int lo = c > 0 ? c - 1 : c;
            int hi = c < C - 1 ? c + 1 : c;
            double best = above[lo];
            for (int k = lo + 1; k <= hi; ++k)
                if (above[k] < best)
                    best = above[k];
            row[c] = er[c] + best;
        }
    }

    // Backtrack from the cheapest bottom cell (leftmost on ties).
    Seam seam(R);
    const double* last = &M[static_cast<size_t>(R - 1) * C];
    int col = 0;
    for (int c = 1; c < C; ++c)
        if (last[c] < last[col])
            col = c;
    seam[R - 1] = col;
    for (int r = R - 2; r >= 0; --r) {
        const double* row = &M[static_cast<size_t>(r) * C];
        int p = seam[r + 1];
        int lo = p > 0 ? p - 1 : p;
        int hi = p < C - 1 ? p + 1 : p;
        int best = lo;
        for (int k = lo + 1; k <= hi; ++k)
            if (row[k] < row[best])
                best = k;
        seam[r] = best;
    }
    return seam;
}

int main()
{
    useProjectDir();
    cv::Mat tower = cv::imread("images/input/Broadway_tower_edit.jpg");
    if (tower.empty()) {
        std::printf("Could not read images/input. Run from the project folder.\n");
        return 1;
    }
    cv::Mat e = computeEnergy(tower);

    // Average of 20 runs after one warm-up run.
    auto timeIt = [](auto f) {
        f();
        auto start = Clock::now();
        for (int i = 0; i < 20; ++i)
            f();
        return std::chrono::duration<double, std::milli>(Clock::now() - start).count() / 20;
    };

    std::printf("serial DP %.2f ms\n", timeIt([&] { findSeamDP(e); }));
    for (int T : {1, 2}) {
        omp_set_num_threads(T);
        std::printf("OpenMP DP %d threads %.2f ms same=%d\n", T,
                    timeIt([&] { dpOmp(e); }), static_cast<int>(dpOmp(e) == findSeamDP(e)));
    }
    return 0;
}
