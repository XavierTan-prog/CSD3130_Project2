// Times each method removing the same number of vertical seams from one image.
// Build in Release before quoting any numbers in the report.
#include <chrono>
#include <exception>
#include <iostream>
#include <string>

#include <opencv2/imgcodecs.hpp>

#include "seam_carving/carve.hpp"
#include "seam_carving/paths.hpp"

int main(int argc, char** argv) {
    sc::useProjectDir();
    if (argc < 3) {
        std::cerr << "Usage: benchmark <input> <seams-to-remove>\n";
        return 1;
    }
    try {
        const cv::Mat img = cv::imread(argv[1], cv::IMREAD_COLOR);
        if (img.empty()) {
            std::cerr << "Could not read image: " << argv[1] << "\n";
            return 1;
        }
        const int seams = std::stoi(argv[2]);
        if (seams < 1 || seams >= img.cols) {
            std::cerr << "seams-to-remove must be between 1 and " << img.cols - 1 << "\n";
            return 1;
        }

        std::cout << "image " << img.cols << "x" << img.rows << ", removing " << seams
                  << " vertical seams\n";
        for (sc::Method method : {sc::Method::DP, sc::Method::Greedy, sc::Method::GraphCut}) {
            const auto start = std::chrono::steady_clock::now();
            const cv::Mat result = sc::carve(img, img.cols - seams, img.rows, method);
            const auto end = std::chrono::steady_clock::now();
            const double ms = std::chrono::duration<double, std::milli>(end - start).count();
            std::cout << sc::methodName(method) << ": " << ms << " ms\n";
            cv::imwrite("bench_" + sc::methodName(method) + ".png", result);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
