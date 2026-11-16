#include <exception>
#include <iostream>
#include <string>

#include <opencv2/imgcodecs.hpp>

#include "seam_carving/carve.hpp"

int main(int argc, char** argv) {
    if (argc < 5) {
        std::cerr << "Usage: seam_carve <input> <output> <width> <height> [dp|greedy|graphcut]\n";
        return 1;
    }
    try {
        const std::string inputPath = argv[1];
        const std::string outputPath = argv[2];
        const int width = std::stoi(argv[3]);
        const int height = std::stoi(argv[4]);
        const sc::Method method = sc::parseMethod(argc > 5 ? argv[5] : "dp");

        const cv::Mat img = cv::imread(inputPath, cv::IMREAD_COLOR);
        if (img.empty()) {
            std::cerr << "Could not read image: " << inputPath << "\n";
            return 1;
        }
        if (width < 1 || width > img.cols || height < 1 || height > img.rows) {
            std::cerr << "Target size must be between 1x1 and " << img.cols << "x" << img.rows << "\n";
            return 1;
        }

        const cv::Mat result = sc::carve(img, width, height, method);
        if (!cv::imwrite(outputPath, result)) {
            std::cerr << "Could not write image: " << outputPath << "\n";
            return 1;
        }
        std::cout << img.cols << "x" << img.rows << " -> " << result.cols << "x" << result.rows
                  << " using " << sc::methodName(method) << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
