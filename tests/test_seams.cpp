#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <random>

#include "seam_carving/carve.hpp"
#include "seam_carving/dp.hpp"
#include "seam_carving/energy.hpp"
#include "seam_carving/graph_cut.hpp"
#include "seam_carving/greedy.hpp"
#include "seam_carving/seam.hpp"
#include "seam_carving/visualize.hpp"

namespace {

// Hand-checked example. The unique minimum seam is columns {0, 1, 1}
// with cost 1 + 2 + 2 = 5.
cv::Mat knownEnergy() {
    return (cv::Mat_<double>(3, 5) << 1, 4, 3, 5, 2,
                                      3, 2, 5, 2, 3,
                                      5, 2, 4, 2, 3);
}

// Deterministic BGR image with varied gradients in both directions.
cv::Mat patternImage() {
    cv::Mat img(7, 9, CV_8UC3);
    for (int r = 0; r < img.rows; ++r) {
        for (int c = 0; c < img.cols; ++c) {
            img.at<cv::Vec3b>(r, c) = cv::Vec3b(static_cast<uchar>((r * 37 + c * 11) % 256),
                                                static_cast<uchar>((r * c * 13) % 256),
                                                static_cast<uchar>((c * 29 + 5) % 256));
        }
    }
    return img;
}

// Exhaustively enumerate the legal continuations of each path on small maps.
double bruteForceSeamCost(const cv::Mat& energy, int row = 0, int previousCol = -1,
                         double cost = 0.0) {
    if (row == energy.rows) return cost;

    double best = std::numeric_limits<double>::infinity();
    for (int col = 0; col < energy.cols; ++col) {
        if (row > 0 && std::abs(col - previousCol) > 1) continue;
        best = std::min(best, bruteForceSeamCost(energy, row + 1, col,
                                                cost + energy.at<double>(row, col)));
    }
    return best;
}

} // namespace

TEST_CASE("computeEnergy of a constant image is zero everywhere") {
    const cv::Mat img(5, 7, CV_8UC3, cv::Scalar(10, 20, 30));
    const cv::Mat energy = sc::computeEnergy(img);
    REQUIRE(energy.type() == CV_64FC1);
    REQUIRE(energy.rows == img.rows);
    REQUIRE(energy.cols == img.cols);
    double minVal = 0.0, maxVal = 0.0;
    cv::minMaxLoc(energy, &minVal, &maxVal);
    REQUIRE(minVal >= -1e-9);
    REQUIRE(maxVal <= 1e-9);
}

TEST_CASE("computeEnergy is high at a vertical edge and zero in flat areas") {
    // Columns 0-3 black, columns 4-7 white: the edge is between 3 and 4.
    cv::Mat img(6, 8, CV_8UC3, cv::Scalar(0, 0, 0));
    img.colRange(4, 8).setTo(cv::Scalar(255, 255, 255));
    const cv::Mat energy = sc::computeEnergy(img);

    double minVal = 0.0;
    cv::minMaxLoc(energy, &minVal);
    REQUIRE(minVal >= 0.0);
    for (int r = 0; r < energy.rows; ++r) {
        REQUIRE(energy.at<double>(r, 3) > 0.0);
        REQUIRE(energy.at<double>(r, 4) > 0.0);
        REQUIRE(energy.at<double>(r, 0) == 0.0); // flat area, far from the edge
        REQUIRE(energy.at<double>(r, 7) == 0.0);
        REQUIRE(energy.at<double>(r, 3) > energy.at<double>(r, 0));
    }
}

TEST_CASE("computeEnergy L1 and L2 return same-size CV_64FC1 maps with L1 >= L2") {
    const cv::Mat img = patternImage();
    const cv::Mat l1 = sc::computeEnergy(img, sc::EnergyOperator::SobelL1);
    const cv::Mat l2 = sc::computeEnergy(img, sc::EnergyOperator::SobelL2);
    for (const cv::Mat* m : {&l1, &l2}) {
        REQUIRE(m->type() == CV_64FC1);
        REQUIRE(m->rows == img.rows);
        REQUIRE(m->cols == img.cols);
    }
    for (int r = 0; r < img.rows; ++r) {
        for (int c = 0; c < img.cols; ++c) {
            REQUIRE(l2.at<double>(r, c) >= 0.0);
            REQUIRE(l1.at<double>(r, c) + 1e-9 >= l2.at<double>(r, c));
        }
    }
}

TEST_CASE("computeEnergy defaults to the Sobel L1 operator") {
    const cv::Mat img = patternImage();
    const cv::Mat byDefault = sc::computeEnergy(img);
    const cv::Mat l1 = sc::computeEnergy(img, sc::EnergyOperator::SobelL1);
    REQUIRE(cv::norm(byDefault, l1, cv::NORM_INF) == 0.0);
}

TEST_CASE("computeEnergy does not modify its input") {
    const cv::Mat img = patternImage();
    const cv::Mat before = img.clone();
    sc::computeEnergy(img, sc::EnergyOperator::SobelL1);
    sc::computeEnergy(img, sc::EnergyOperator::SobelL2);
    REQUIRE(cv::norm(img, before, cv::NORM_INF) == 0.0);
}

TEST_CASE("computeEnergy rejects invalid input") {
    REQUIRE_THROWS_AS(sc::computeEnergy(cv::Mat()), cv::Exception);
    REQUIRE_THROWS_AS(sc::computeEnergy(cv::Mat(4, 4, CV_8UC1, cv::Scalar(0))), cv::Exception);
    REQUIRE_THROWS_AS(sc::computeEnergy(cv::Mat(4, 4, CV_64FC3, cv::Scalar(0, 0, 0))),
                      cv::Exception);
    const cv::Mat img(4, 4, CV_8UC3, cv::Scalar(1, 2, 3));
    REQUIRE_THROWS_AS(sc::computeEnergy(img, static_cast<sc::EnergyOperator>(99)), cv::Exception);
}

TEST_CASE("isValidSeam accepts connected seams only") {
    REQUIRE(sc::isValidSeam({0, 1, 1}, 3, 5));
    REQUIRE(sc::isValidSeam({4, 3, 4}, 3, 5));
    REQUIRE_FALSE(sc::isValidSeam({0, 2, 2}, 3, 5)); // jumps two columns
    REQUIRE_FALSE(sc::isValidSeam({0, 1}, 3, 5));    // too short
    REQUIRE_FALSE(sc::isValidSeam({0, 1, 5}, 3, 5)); // out of range
}

TEST_CASE("seamCost sums the energy along the seam") {
    REQUIRE(sc::seamCost(knownEnergy(), {0, 1, 1}) == 5.0);
}

TEST_CASE("removeVerticalSeam removes one pixel per row") {
    const cv::Mat img = (cv::Mat_<uchar>(2, 3) << 1, 2, 3,
                                                  4, 5, 6);
    const cv::Mat out = sc::removeVerticalSeam(img, {1, 0});
    REQUIRE(out.rows == 2);
    REQUIRE(out.cols == 2);
    REQUIRE(out.at<uchar>(0, 0) == 1);
    REQUIRE(out.at<uchar>(0, 1) == 3);
    REQUIRE(out.at<uchar>(1, 0) == 5);
    REQUIRE(out.at<uchar>(1, 1) == 6);
}

TEST_CASE("every method returns a valid seam") {
    const cv::Mat energy = knownEnergy();
    for (sc::Method m : {sc::Method::DP, sc::Method::Greedy, sc::Method::GraphCut}) {
        REQUIRE(sc::isValidSeam(sc::findSeam(energy, m), energy.rows, energy.cols));
    }
}

TEST_CASE("carve reaches the target size") {
    const cv::Mat img(6, 8, CV_8UC3, cv::Scalar(10, 20, 30));
    const cv::Mat out = sc::carve(img, 5, 4, sc::Method::DP);
    REQUIRE(out.cols == 5);
    REQUIRE(out.rows == 4);
}

TEST_CASE("drawSeam paints only the seam pixels") {
    const cv::Mat img(3, 4, CV_8UC3, cv::Scalar(10, 20, 30));
    const cv::Mat out = sc::drawSeam(img, {0, 1, 2});
    REQUIRE(out.at<cv::Vec3b>(0, 0) == cv::Vec3b(0, 0, 255));
    REQUIRE(out.at<cv::Vec3b>(1, 1) == cv::Vec3b(0, 0, 255));
    REQUIRE(out.at<cv::Vec3b>(2, 2) == cv::Vec3b(0, 0, 255));
    REQUIRE(out.at<cv::Vec3b>(0, 1) == cv::Vec3b(10, 20, 30));
    REQUIRE(img.at<cv::Vec3b>(0, 0) == cv::Vec3b(10, 20, 30)); // input untouched
}

TEST_CASE("sideBySide fits both images on one canvas") {
    const cv::Mat left(150, 200, CV_8UC3, cv::Scalar(255, 0, 0));
    const cv::Mat right(120, 150, CV_8UC3, cv::Scalar(0, 255, 0));
    const cv::Mat canvas = sc::sideBySide(left, "left", right, "right");
    REQUIRE(canvas.type() == CV_8UC3);
    REQUIRE(canvas.cols > left.cols + right.cols);
    REQUIRE(canvas.rows > left.rows);
}

TEST_CASE("sideBySide shrinks large images to fit on screen") {
    const cv::Mat left(968, 1428, CV_8UC3, cv::Scalar(255, 0, 0));
    const cv::Mat right(726, 1071, CV_8UC3, cv::Scalar(0, 255, 0));
    const cv::Mat canvas = sc::sideBySide(left, "left", right, "right");
    REQUIRE(canvas.cols <= 1300);
    REQUIRE(canvas.rows <= 700);
}

TEST_CASE("Graph cut finds the known minimum-energy seam", "[graphcut]") {
    const cv::Mat energy = knownEnergy();
    const sc::Seam seam = sc::findSeamGraphCut(energy);
    REQUIRE(seam == sc::Seam{0, 1, 1});
    REQUIRE(sc::seamCost(energy, seam) == 5.0);
}

TEST_CASE("Graph cut keeps adjacent rows connected", "[graphcut]") {
    // The two zero-energy pixels are too far apart to share a seam.
    const cv::Mat energy = (cv::Mat_<double>(2, 4) << 0, 7, 3, 7,
                                                    7, 7, 7, 0);
    REQUIRE(sc::findSeamGraphCut(energy) == sc::Seam{2, 3});

    cv::Mat flipped;
    cv::flip(energy, flipped, 0);
    REQUIRE(sc::findSeamGraphCut(flipped) == sc::Seam{3, 2});
}

TEST_CASE("Graph cut can select either boundary of a two-column map", "[graphcut]") {
    const cv::Mat energy = (cv::Mat_<double>(3, 2) << 9, 1,
                                                    8, 2,
                                                    7, 3);
    REQUIRE(sc::findSeamGraphCut(energy) == sc::Seam{1, 1, 1});

    cv::Mat flipped;
    cv::flip(energy, flipped, 1);
    REQUIRE(sc::findSeamGraphCut(flipped) == sc::Seam{0, 0, 0});
}

TEST_CASE("Graph cut handles one-row and one-column maps", "[graphcut]") {
    SECTION("One row with small fractional energies") {
        const cv::Mat energy = (cv::Mat_<double>(1, 3) << 8e-20, 4e-20, 1e-20);
        REQUIRE(sc::findSeamGraphCut(energy) == sc::Seam{2});
    }
    SECTION("One column") {
        const cv::Mat energy = (cv::Mat_<double>(3, 1) << 9, 0, 2);
        REQUIRE(sc::findSeamGraphCut(energy) == sc::Seam{0, 0, 0});
    }
    SECTION("One pixel with the largest finite energy") {
        const cv::Mat energy(1, 1, CV_64FC1, cv::Scalar(std::numeric_limits<double>::max()));
        REQUIRE(sc::findSeamGraphCut(energy) == sc::Seam{0});
    }
}

TEST_CASE("Graph cut finds a valid optimum on tied and zero-cost maps", "[graphcut]") {
    for (double value : {2.0, 0.0}) {
        const cv::Mat energy(3, 4, CV_64FC1, cv::Scalar(value));
        const sc::Seam seam = sc::findSeamGraphCut(energy);
        REQUIRE(sc::isValidSeam(seam, energy.rows, energy.cols));
        REQUIRE(sc::seamCost(energy, seam) == energy.rows * value);
    }
}

TEST_CASE("Graph cut matches exhaustive seam search on small maps", "[graphcut]") {
    std::mt19937 rng(3130);
    std::uniform_int_distribution<int> energyValue(0, 12);
    for (int height = 1; height <= 5; ++height) {
        for (int width = 1; width <= 5; ++width) {
            for (int trial = 0; trial < 20; ++trial) {
                CAPTURE(height, width, trial);
                cv::Mat_<double> energy(height, width);
                for (double& cost : energy) cost = energyValue(rng) * 0.25;

                const sc::Seam seam = sc::findSeamGraphCut(energy);
                REQUIRE(sc::isValidSeam(seam, height, width));
                REQUIRE(sc::seamCost(energy, seam) == bruteForceSeamCost(energy));
            }
        }
    }
}

TEST_CASE("Graph cut rejects invalid energy maps", "[graphcut]") {
    REQUIRE_THROWS_AS(sc::findSeamGraphCut(cv::Mat()), cv::Exception);
    REQUIRE_THROWS_AS(sc::findSeamGraphCut(cv::Mat(2, 2, CV_8UC1, cv::Scalar(1))), cv::Exception);
    for (double invalid : {-1.0, std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::quiet_NaN()}) {
        const cv::Mat energy(2, 2, CV_64FC1, cv::Scalar(invalid));
        REQUIRE_THROWS_AS(sc::findSeamGraphCut(energy), cv::Exception);
    }
}

TEST_CASE("Carve removes graph-cut seams in both directions", "[graphcut]") {
    // The rightmost column is the only zero-cost vertical seam with Sobel L1.
    cv::Mat img(3, 3, CV_8UC3, cv::Scalar::all(50));
    img.col(2).setTo(cv::Scalar::all(200));
    for (int row = 0; row < img.rows; ++row) {
        const auto grey = static_cast<uchar>(10 * (row + 1));
        img.at<cv::Vec3b>(row, 0) = cv::Vec3b(grey, grey, grey);
    }

    const cv::Mat expectedVertical = img.colRange(0, 2);
    const cv::Mat vertical = sc::carve(img, 2, img.rows, sc::Method::GraphCut);
    REQUIRE(vertical.size() == expectedVertical.size());
    REQUIRE(cv::norm(vertical, expectedVertical, cv::NORM_INF) == 0.0);

    const cv::Mat transposed = img.t();
    const cv::Mat expectedHorizontal = expectedVertical.t();
    const cv::Mat horizontal = sc::carve(transposed, transposed.cols, 2, sc::Method::GraphCut);
    REQUIRE(horizontal.size() == expectedHorizontal.size());
    REQUIRE(cv::norm(horizontal, expectedHorizontal, cv::NORM_INF) == 0.0);
}

// DP and greedy tests stay hidden while those methods are placeholders.

TEST_CASE("DP finds the optimal seam", "[.][dp]") {
    const cv::Mat energy = knownEnergy();
    const sc::Seam seam = sc::findSeamDP(energy);
    REQUIRE(seam == sc::Seam{0, 1, 1});
    REQUIRE(sc::seamCost(energy, seam) == 5.0);
}

TEST_CASE("greedy never beats DP", "[.][greedy]") {
    const cv::Mat energy = knownEnergy();
    REQUIRE(sc::seamCost(energy, sc::findSeamGreedy(energy)) >=
            sc::seamCost(energy, sc::findSeamDP(energy)));
}
