// seam_carve: resizes an image and shows the original next to the result.
//
// With no arguments it asks which image to use, the target size and the
// method. With arguments it runs straight away:
//   seam_carve <input> <output> <width> <height> [dp|greedy|graphcut] [--no-show]
#include <algorithm>
#include <cctype>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#include <opencv2/imgcodecs.hpp>

#include "seam_carving/carve.hpp"
#include "seam_carving/paths.hpp"
#include "seam_carving/visualize.hpp"

namespace fs = std::filesystem;

namespace {

const char* const kInputDir = "images/input";
const char* const kOutputDir = "images/output";

std::string sizeText(const cv::Mat& img) {
    return std::to_string(img.cols) + "x" + std::to_string(img.rows);
}

// Removes surrounding spaces and quotes (dragging a file into the console
// pastes its path in quotes).
std::string clean(std::string text) {
    const auto isJunk = [](unsigned char c) { return std::isspace(c) || c == '"'; };
    while (!text.empty() && isJunk(static_cast<unsigned char>(text.front()))) text.erase(text.begin());
    while (!text.empty() && isJunk(static_cast<unsigned char>(text.back()))) text.pop_back();
    return text;
}

// Prints the prompt and reads one line. An empty answer means "use the default".
std::string ask(const std::string& prompt) {
    std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) throw std::runtime_error("no input available");
    return clean(line);
}

bool parseInt(const std::string& text, int& value) {
    try {
        size_t used = 0;
        value = std::stoi(text, &used);
        return used == text.size();
    } catch (const std::exception&) {
        return false;
    }
}

int askInt(const std::string& label, int low, int high, int fallback) {
    for (;;) {
        const std::string answer = ask(label + " (" + std::to_string(low) + "-" +
                                       std::to_string(high) + ") [" + std::to_string(fallback) + "]: ");
        if (answer.empty()) return fallback;
        int value = 0;
        if (parseInt(answer, value) && value >= low && value <= high) return value;
        std::cout << "  Please enter a whole number from " << low << " to " << high << ".\n";
    }
}

sc::Method askMethod() {
    for (;;) {
        const std::string answer = ask("Method (dp, greedy, graphcut) [dp]: ");
        if (answer.empty()) return sc::Method::DP;
        try {
            return sc::parseMethod(answer);
        } catch (const std::invalid_argument&) {
            std::cout << "  Please enter dp, greedy or graphcut.\n";
        }
    }
}

std::vector<fs::path> listImages(const fs::path& dir) {
    std::vector<fs::path> images;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp") {
            images.push_back(entry.path());
        }
    }
    std::sort(images.begin(), images.end());
    return images;
}

// Asks which image to use until one loads. Returns its path; fills `img`.
fs::path askImage(cv::Mat& img) {
    for (;;) {
        const std::vector<fs::path> images = listImages(kInputDir);
        std::string prompt;
        if (images.empty()) {
            std::cout << "No images found in " << kInputDir << ".\n";
            prompt = "Image file path: ";
        } else {
            std::cout << "Images in " << kInputDir << ":\n";
            for (size_t i = 0; i < images.size(); ++i) {
                std::cout << "  " << (i + 1) << ") " << images[i].filename().string() << "\n";
            }
            prompt = "Choose an image (number, or a file path) [1]: ";
        }

        const std::string answer = ask(prompt);
        fs::path chosen;
        int number = 0;
        if (answer.empty() && !images.empty()) {
            chosen = images.front();
        } else if (parseInt(answer, number) && number >= 1 &&
                   number <= static_cast<int>(images.size())) {
            chosen = images[static_cast<size_t>(number - 1)];
        } else {
            chosen = answer;
        }

        std::error_code ec;
        if (fs::is_regular_file(chosen, ec)) img = cv::imread(chosen.string(), cv::IMREAD_COLOR);
        if (!img.empty()) return chosen;
        std::cout << "  Could not read that image. Try again.\n\n";
    }
}

int resizeAndShow(const cv::Mat& img, const fs::path& outputPath, int width, int height,
                  sc::Method method, bool show) {
    const cv::Mat result = sc::carve(img, width, height, method);

    std::error_code ec;
    if (outputPath.has_parent_path()) fs::create_directories(outputPath.parent_path(), ec);
    if (!cv::imwrite(outputPath.string(), result)) {
        std::cerr << "Could not write image: " << outputPath.string() << "\n";
        return 1;
    }
    std::cout << sizeText(img) << " -> " << sizeText(result) << " using "
              << sc::methodName(method) << ", saved to " << outputPath.string() << "\n";

    if (show) {
        const cv::Mat view = sc::sideBySide(
            img, "Original " + sizeText(img),
            result, sc::methodName(method) + " " + sizeText(result));
        std::cout << "Press any key in the image window to close it.\n";
        sc::showWindow("Seam carving", view);
    }
    return 0;
}

int runInteractive(bool show) {
    cv::Mat img;
    const fs::path inputPath = askImage(img);
    std::cout << inputPath.filename().string() << " is " << sizeText(img) << ".\n";

    const int width = askInt("Target width ", 1, img.cols, std::max(1, img.cols * 3 / 4));
    const int height = askInt("Target height", 1, img.rows, std::max(1, img.rows * 3 / 4));
    const sc::Method method = askMethod();

    const fs::path outputPath = fs::path(kOutputDir) /
        (inputPath.stem().string() + "_" + sc::methodName(method) + ".png");
    return resizeAndShow(img, outputPath, width, height, method, show);
}

int runWithArguments(const std::vector<std::string>& args, bool show) {
    const std::string inputPath = args[0];
    const std::string outputPath = args[1];
    int width = 0;
    int height = 0;
    if (!parseInt(args[2], width) || !parseInt(args[3], height)) {
        std::cerr << "Width and height must be whole numbers.\n";
        return 1;
    }
    const sc::Method method = sc::parseMethod(args.size() > 4 ? args[4] : "dp");

    const cv::Mat img = cv::imread(inputPath, cv::IMREAD_COLOR);
    if (img.empty()) {
        std::cerr << "Could not read image: " << inputPath << "\n";
        return 1;
    }
    if (width < 1 || width > img.cols || height < 1 || height > img.rows) {
        std::cerr << "Target size must be between 1x1 and " << sizeText(img) << "\n";
        return 1;
    }
    return resizeAndShow(img, outputPath, width, height, method, show);
}

} // namespace

int main(int argc, char** argv) {
    sc::useProjectDir();

    // "--no-show" skips the preview window; everything else is positional.
    bool show = true;
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--no-show") show = false;
        else args.push_back(arg);
    }

    try {
        if (args.empty()) return runInteractive(show);
        if (args.size() < 4) {
            std::cerr << "Usage: seam_carve                      (asks for everything)\n"
                         "       seam_carve <input> <output> <width> <height> "
                         "[dp|greedy|graphcut] [--no-show]\n";
            return 1;
        }
        return runWithArguments(args, show);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
