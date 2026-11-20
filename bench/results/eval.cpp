// Results harness: figures and timings for the report.
// Not part of the assignment program. It only calls our library
// (computeEnergy, findSeam*, removeVerticalSeam) and writes the figures to
// images/output/results/ (ignored by Git). Run bench/results/run_results.bat.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "seam_carving/carve.hpp"
#include "seam_carving/dp.hpp"
#include "seam_carving/energy.hpp"
#include "seam_carving/graph_cut.hpp"
#include "seam_carving/greedy.hpp"
#include "seam_carving/paths.hpp"
#include "seam_carving/seam.hpp"

using namespace sc;
using Clock = std::chrono::steady_clock;

static const std::string OUT = "images/output/results/";

static double msBetween(Clock::time_point a, Clock::time_point b)
{
    return std::chrono::duration<double, std::milli>(b - a).count();
}

// Average time of f() in ms over `reps` runs, after one warm-up run.
template <class F>
static double timeIt(F f, int reps)
{
    f();
    auto start = Clock::now();
    for (int i = 0; i < reps; ++i)
        f();
    return msBetween(start, Clock::now()) / reps;
}

// Energy map scaled for display: log(e + 1), stretched to 0..255.
static cv::Mat energyVis(const cv::Mat& energy)
{
    cv::Mat logE, vis;
    cv::log(energy + 1.0, logE);
    cv::normalize(logE, vis, 0, 255, cv::NORM_MINMAX);
    vis.convertTo(vis, CV_8U);
    return vis;
}

// Removes n vertical seams with method m. `mask` marks every removed pixel in
// the original image's coordinates, and `costs` gets each seam's energy.
static cv::Mat carveTrack(const cv::Mat& in, int n, Method m, cv::Mat& mask,
                          std::vector<double>& costs)
{
    cv::Mat img = in.clone();

    // idx(r, c) = original column of the pixel now at (r, c).
    cv::Mat idx(in.rows, in.cols, CV_32S);
    for (int r = 0; r < in.rows; ++r)
        for (int c = 0; c < in.cols; ++c)
            idx.at<int>(r, c) = c;

    mask = cv::Mat::zeros(in.rows, in.cols, CV_8U);
    for (int k = 0; k < n; ++k) {
        cv::Mat energy = computeEnergy(img);
        Seam seam = findSeam(energy, m);
        costs.push_back(seamCost(energy, seam));

        for (int r = 0; r < img.rows; ++r)
            mask.at<uchar>(r, idx.at<int>(r, seam[r])) = 255;

        img = removeVerticalSeam(img, seam);
        idx = removeVerticalSeam(idx, seam);
    }
    return img;
}

// Paints the masked pixels red.
static cv::Mat overlay(const cv::Mat& img, const cv::Mat& mask)
{
    cv::Mat out = img.clone();
    for (int r = 0; r < out.rows; ++r)
        for (int c = 0; c < out.cols; ++c)
            if (mask.at<uchar>(r, c))
                out.at<cv::Vec3b>(r, c) = cv::Vec3b(0, 0, 255);
    return out;
}

// Experiment: DP that keeps only two rows of costs, plus a 1-byte parent
// offset (-1, 0 or +1) per pixel for backtracking.
static Seam dpTwoRow(const cv::Mat& e)
{
    const int R = e.rows, C = e.cols;
    std::vector<double> prev(C), cur(C);
    std::vector<signed char> parent(static_cast<size_t>(R) * C, 0);

    for (int c = 0; c < C; ++c)
        prev[c] = e.at<double>(0, c);

    for (int r = 1; r < R; ++r) {
        const double* er = e.ptr<double>(r);
        for (int c = 0; c < C; ++c) {
            int lo = c > 0 ? c - 1 : c;
            int hi = c < C - 1 ? c + 1 : c;
            int best = lo;
            for (int k = lo + 1; k <= hi; ++k)
                if (prev[k] < prev[best])
                    best = k;
            cur[c] = er[c] + prev[best];
            parent[static_cast<size_t>(r) * C + c] = static_cast<signed char>(best - c);
        }
        std::swap(prev, cur);
    }

    Seam seam(R);
    int col = 0;
    for (int c = 1; c < C; ++c)
        if (prev[c] < prev[col])
            col = c;
    seam[R - 1] = col;
    for (int r = R - 1; r > 0; --r) {
        col += parent[static_cast<size_t>(r) * C + col];
        seam[r - 1] = col;
    }
    return seam;
}

// Experiment: full-table DP where each row is split across T new threads
// (cells in one row only depend on the row above).
static Seam dpThreaded(const cv::Mat& e, int T)
{
    const int R = e.rows, C = e.cols;
    std::vector<double> M(static_cast<size_t>(R) * C);

    for (int c = 0; c < C; ++c)
        M[c] = e.at<double>(0, c);

    for (int r = 1; r < R; ++r) {
        const double* er = e.ptr<double>(r);
        const double* above = &M[static_cast<size_t>(r - 1) * C];
        double* row = &M[static_cast<size_t>(r) * C];

        auto work = [&](int a, int b) {
            for (int c = a; c < b; ++c) {
                int lo = c > 0 ? c - 1 : c;
                int hi = c < C - 1 ? c + 1 : c;
                double best = above[lo];
                for (int k = lo + 1; k <= hi; ++k)
                    if (above[k] < best)
                        best = above[k];
                row[c] = er[c] + best;
            }
        };

        std::vector<std::thread> threads;
        int step = (C + T - 1) / T;
        for (int t = 0; t < T; ++t) {
            int a = t * step, b = std::min(C, a + step);
            if (a < b)
                threads.emplace_back(work, a, b);
        }
        for (auto& th : threads)
            th.join();
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
    std::filesystem::create_directories(OUT);

    cv::Mat tower = cv::imread("images/input/Broadway_tower_edit.jpg");
    cv::Mat sample = cv::imread("images/input/sample.png");
    if (tower.empty() || sample.empty()) {
        std::printf("Could not read images/input. Run from the project folder.\n");
        return 1;
    }

    // 1. Energy maps (L1 default and L2) for both images.
    const std::pair<std::string, cv::Mat> images[] = {{"tower", tower}, {"sample", sample}};
    for (const auto& [name, img] : images) {
        cv::imwrite(OUT + "energy_" + name + ".png", energyVis(computeEnergy(img)));
        cv::imwrite(OUT + "energyL2_" + name + ".png",
                    energyVis(computeEnergy(img, EnergyOperator::SobelL2)));
    }

    // 2. Tower: remove 25% of the width with DP and greedy.
    //    Plain scaling and cropping are saved for comparison.
    const int n = tower.cols / 4;
    std::printf("tower %dx%d removing %d\n", tower.cols, tower.rows, n);
    for (Method m : {Method::DP, Method::Greedy}) {
        cv::Mat mask;
        std::vector<double> costs;
        auto start = Clock::now();
        cv::Mat out = carveTrack(tower, n, m, mask, costs);
        double t = msBetween(start, Clock::now());

        double sum = 0;
        for (double c : costs)
            sum += c;
        std::printf("tower %s total %.0f ms, first seam cost %.1f, mean seam cost %.1f\n",
                    methodName(m).c_str(), t, costs[0], sum / costs.size());

        cv::imwrite(OUT + "tower_" + methodName(m) + ".png", out);
        cv::imwrite(OUT + "tower_seams_" + methodName(m) + ".png", overlay(tower, mask));
    }
    {
        cv::Mat scaled;
        cv::resize(tower, scaled, cv::Size(tower.cols - n, tower.rows), 0, 0, cv::INTER_AREA);
        cv::imwrite(OUT + "tower_scale.png", scaled);
        cv::imwrite(OUT + "tower_crop.png", tower(cv::Rect(n / 2, 0, tower.cols - n, tower.rows)).clone());
    }

    // 3. Sample: remove 50 seams with all three methods.
    const int ns = 50;
    for (Method m : {Method::DP, Method::Greedy, Method::GraphCut}) {
        cv::Mat mask;
        std::vector<double> costs;
        auto start = Clock::now();
        cv::Mat out = carveTrack(sample, ns, m, mask, costs);
        double t = msBetween(start, Clock::now());

        double sum = 0;
        for (double c : costs)
            sum += c;
        std::printf("sample %s total %.1f ms, first seam cost %.1f, mean %.1f\n",
                    methodName(m).c_str(), t, costs[0], sum / costs.size());

        cv::imwrite(OUT + "sample_" + methodName(m) + ".png", out);
        cv::imwrite(OUT + "sample_seams_" + methodName(m) + ".png", overlay(sample, mask));
    }
    {
        cv::Mat scaled;
        cv::resize(sample, scaled, cv::Size(150, 150), 0, 0, cv::INTER_AREA);
        cv::imwrite(OUT + "sample_scale.png", scaled);
    }

    // 4. First seam on the sample with each method.
    {
        cv::Mat e = computeEnergy(sample);
        for (Method m : {Method::DP, Method::Greedy, Method::GraphCut}) {
            Seam s = findSeam(e, m);
            std::printf("sample first seam %s cost %.2f start col %d end col %d\n",
                        methodName(m).c_str(), seamCost(e, s), s.front(), s.back());
        }
    }

    // 5. Time per seam at five sizes (tower scaled down). Graph cut only up
    //    to width 400 here; gcbig.cpp times the two largest sizes. -1 = not run.
    std::printf("\nsize, energy_ms, dp_ms, greedy_ms, graphcut_ms, dp_cost, greedy_cost, gc_cost\n");
    for (int w : {100, 200, 400, 714, 1428}) {
        int h = static_cast<int>(std::lround(tower.rows * static_cast<double>(w) / tower.cols));
        cv::Mat im;
        if (w == tower.cols)
            im = tower;
        else
            cv::resize(tower, im, cv::Size(w, h), 0, 0, cv::INTER_AREA);

        cv::Mat e = computeEnergy(im);
        int reps = w <= 400 ? 20 : 5;
        double tEnergy = timeIt([&] { computeEnergy(im); }, reps);
        double tDP = timeIt([&] { findSeamDP(e); }, reps);
        double tGreedy = timeIt([&] { findSeamGreedy(e); }, reps * 10);

        double tGC = -1, costGC = -1;
        if (w <= 400) {
            tGC = timeIt([&] { findSeamGraphCut(e); }, w <= 200 ? 5 : 1);
            costGC = seamCost(e, findSeamGraphCut(e));
        }

        std::printf("%dx%d, %.3f, %.3f, %.4f, %.1f, %.1f, %.1f, %.1f\n", w, h,
                    tEnergy, tDP, tGreedy, tGC,
                    seamCost(e, findSeamDP(e)), seamCost(e, findSeamGreedy(e)), costGC);
    }

    // 6. Improvement experiments on the full tower (not in our source code).
    {
        cv::Mat e = computeEnergy(tower);

        double tFull = timeIt([&] { findSeamDP(e); }, 10);
        double tTwoRow = timeIt([&] { dpTwoRow(e); }, 10);
        double mbFull = tower.total() * 8 / 1e6;
        double mbTwoRow = tower.total() * 1 / 1e6 + tower.cols * 16 / 1e6;
        std::printf("\nDP full table %.2f ms (%.1f MB), two-row+parent %.2f ms (%.1f MB); same seam=%d\n",
                    tFull, mbFull, tTwoRow, mbTwoRow, static_cast<int>(findSeamDP(e) == dpTwoRow(e)));

        for (int T : {1, 2, 4, 8}) {
            double t = timeIt([&] { dpThreaded(e, T); }, 5);
            std::printf("DP row-parallel %d threads: %.2f ms same=%d\n",
                        T, t, static_cast<int>(dpThreaded(e, T) == findSeamDP(e)));
        }

        // OpenCV's own threads for the Sobel energy map (0 = OpenCV default).
        for (int T : {1, 4, 0}) {
            cv::setNumThreads(T == 0 ? -1 : T);
            double t = timeIt([&] { computeEnergy(tower); }, 10);
            std::printf("energy with cv threads=%d: %.2f ms\n", T, t);
        }
        cv::setNumThreads(-1);
    }
    return 0;
}
