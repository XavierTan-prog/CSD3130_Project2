#include <catch2/catch_test_macros.hpp>

#include "seam_carving/carve.hpp"
#include "seam_carving/dp.hpp"
#include "seam_carving/graph_cut.hpp"
#include "seam_carving/greedy.hpp"
#include "seam_carving/seam.hpp"

namespace {

// Hand-checked example. The unique minimum seam is columns {0, 1, 1}
// with cost 1 + 2 + 2 = 5.
cv::Mat knownEnergy() {
    return (cv::Mat_<double>(3, 5) << 1, 4, 3, 5, 2,
                                      3, 2, 5, 2, 3,
                                      5, 2, 4, 2, 3);
}

} // namespace

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

// The tests below are hidden (the "[.]" tag) because the algorithms are
// still placeholders. Delete "[.]" from a test once you implement that
// method, and it will run in CI with the rest.

TEST_CASE("DP finds the optimal seam", "[.][dp]") {
    const cv::Mat energy = knownEnergy();
    const sc::Seam seam = sc::findSeamDP(energy);
    REQUIRE(seam == sc::Seam{0, 1, 1});
    REQUIRE(sc::seamCost(energy, seam) == 5.0);
}

TEST_CASE("graph cut matches the DP optimum", "[.][graphcut]") {
    const cv::Mat energy = knownEnergy();
    REQUIRE(sc::seamCost(energy, sc::findSeamGraphCut(energy)) == 5.0);
}

TEST_CASE("greedy never beats DP", "[.][greedy]") {
    const cv::Mat energy = knownEnergy();
    REQUIRE(sc::seamCost(energy, sc::findSeamGreedy(energy)) >=
            sc::seamCost(energy, sc::findSeamDP(energy)));
}
