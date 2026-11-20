#include <catch2/catch_test_macros.hpp>

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

// The tests below are hidden (the "[.]" tag) because the algorithms are
// still placeholders. Delete "[.]" from a test once you implement that
// method, and it will run in CI with the rest.

TEST_CASE("DP finds the optimal seam", "[dp]") {
    const cv::Mat energy = knownEnergy();
    const sc::Seam seam = sc::findSeamDP(energy);
    REQUIRE(seam == sc::Seam{0, 1, 1});
    REQUIRE(sc::seamCost(energy, seam) == 5.0);
}

TEST_CASE("graph cut matches the DP optimum", "[graphcut]") {
    const cv::Mat energy = knownEnergy();
    REQUIRE(sc::seamCost(energy, sc::findSeamGraphCut(energy)) == 5.0);
}

TEST_CASE("greedy never beats DP", "[greedy]") {
    const cv::Mat energy = knownEnergy();
    REQUIRE(sc::seamCost(energy, sc::findSeamGreedy(energy)) >=
            sc::seamCost(energy, sc::findSeamDP(energy)));
}

// Edge cases and a case where greedy is strictly worse than DP.

namespace {

// Greedy starts at the cheap 1 in the top-left and gets stuck among the 9s
// (1 + 9 + 9 = 19). The optimum starts at a 5 and drops to the 1s (5+1+1 = 7).
cv::Mat greedyTrapEnergy() {
    return (cv::Mat_<double>(3, 3) << 1, 5, 5,
                                      9, 9, 1,
                                      9, 9, 1);
}

// Brute force over every seam, for checking DP on small maps.
double bruteForceMinCost(const cv::Mat& e, int r, int c) {
    const double here = e.at<double>(r, c);
    if (r == e.rows - 1) return here;
    double best = -1.0;
    for (int k = c - 1; k <= c + 1; ++k) {
        if (k < 0 || k >= e.cols) continue;
        const double sub = bruteForceMinCost(e, r + 1, k);
        if (best < 0.0 || sub < best) best = sub;
    }
    return here + best;
}

} // namespace

TEST_CASE("greedy can be strictly worse than DP", "[dp][greedy]") {
    const cv::Mat energy = greedyTrapEnergy();
    const sc::Seam dp = sc::findSeamDP(energy);
    const sc::Seam greedy = sc::findSeamGreedy(energy);
    REQUIRE(sc::isValidSeam(dp, energy.rows, energy.cols));
    REQUIRE(sc::isValidSeam(greedy, energy.rows, energy.cols));
    REQUIRE(sc::seamCost(energy, dp) == 7.0);
    REQUIRE(sc::seamCost(energy, greedy) == 19.0);
}

TEST_CASE("DP matches brute force on a pattern image", "[dp]") {
    const cv::Mat energy = sc::computeEnergy(patternImage());
    double best = -1.0;
    for (int c = 0; c < energy.cols; ++c) {
        const double cost = bruteForceMinCost(energy, 0, c);
        if (best < 0.0 || cost < best) best = cost;
    }
    const sc::Seam seam = sc::findSeamDP(energy);
    REQUIRE(sc::isValidSeam(seam, energy.rows, energy.cols));
    REQUIRE(sc::seamCost(energy, seam) == best);
}

TEST_CASE("DP and greedy handle a one-column image", "[dp][greedy]") {
    const cv::Mat energy = (cv::Mat_<double>(4, 1) << 3, 1, 4, 1);
    REQUIRE(sc::findSeamDP(energy) == sc::Seam{0, 0, 0, 0});
    REQUIRE(sc::findSeamGreedy(energy) == sc::Seam{0, 0, 0, 0});
}

TEST_CASE("DP and greedy handle a one-row image", "[dp][greedy]") {
    const cv::Mat energy = (cv::Mat_<double>(1, 5) << 4, 2, 7, 1, 3);
    REQUIRE(sc::findSeamDP(energy) == sc::Seam{3});
    REQUIRE(sc::findSeamGreedy(energy) == sc::Seam{3});
}

TEST_CASE("DP and greedy break ties to the leftmost column", "[dp][greedy]") {
    const cv::Mat energy(3, 4, CV_64FC1, cv::Scalar(2.0));
    REQUIRE(sc::findSeamDP(energy) == sc::Seam{0, 0, 0});
    REQUIRE(sc::findSeamGreedy(energy) == sc::Seam{0, 0, 0});
}

TEST_CASE("graph cut returns a valid seam with the DP cost", "[graphcut]") {
    const cv::Mat pattern = sc::computeEnergy(patternImage());
    for (const cv::Mat& energy : {knownEnergy(), greedyTrapEnergy(), pattern}) {
        const sc::Seam cut = sc::findSeamGraphCut(energy);
        REQUIRE(sc::isValidSeam(cut, energy.rows, energy.cols));
        REQUIRE(sc::seamCost(energy, cut) == sc::seamCost(energy, sc::findSeamDP(energy)));
    }
}

TEST_CASE("graph cut can put the seam in the last column", "[graphcut]") {
    // The cheapest pixels are all in the last column.
    const cv::Mat energy = (cv::Mat_<double>(3, 3) << 9, 9, 1,
                                                      9, 9, 1,
                                                      9, 9, 1);
    REQUIRE(sc::findSeamGraphCut(energy) == sc::Seam{2, 2, 2});
}

TEST_CASE("graph cut handles one-column and one-row images", "[graphcut]") {
    const cv::Mat column = (cv::Mat_<double>(4, 1) << 3, 1, 4, 1);
    REQUIRE(sc::findSeamGraphCut(column) == sc::Seam{0, 0, 0, 0});
    const cv::Mat row = (cv::Mat_<double>(1, 5) << 4, 2, 7, 1, 3);
    REQUIRE(sc::findSeamGraphCut(row) == sc::Seam{3});
}

TEST_CASE("graph cut rejects invalid energy maps", "[graphcut]") {
    REQUIRE_THROWS_AS(sc::findSeamGraphCut(cv::Mat()), cv::Exception);
    REQUIRE_THROWS_AS(sc::findSeamGraphCut(cv::Mat(3, 3, CV_8UC1, cv::Scalar(0))), cv::Exception);
}

TEST_CASE("DP and greedy reject invalid energy maps", "[dp][greedy]") {
    REQUIRE_THROWS_AS(sc::findSeamDP(cv::Mat()), cv::Exception);
    REQUIRE_THROWS_AS(sc::findSeamGreedy(cv::Mat()), cv::Exception);
    const cv::Mat wrongType(3, 3, CV_8UC1, cv::Scalar(0));
    REQUIRE_THROWS_AS(sc::findSeamDP(wrongType), cv::Exception);
    REQUIRE_THROWS_AS(sc::findSeamGreedy(wrongType), cv::Exception);
}

TEST_CASE("DP and greedy pick the leftmost cell on a tie while backtracking", "[dp][greedy]") {
    // M = 1 1 5 / 6 1 6 / 6 1 6. DP ends at column 1 (M = 1), and when
    // backtracking into row 0 columns 0 and 1 tie at M = 1, so it takes 0.
    // Greedy also starts at the leftmost of the two 1s in the top row.
    const cv::Mat energy = (cv::Mat_<double>(3, 3) << 1, 1, 5,
                                                      5, 0, 5,
                                                      5, 0, 5);
    REQUIRE(sc::findSeamDP(energy) == sc::Seam{0, 1, 1});
    REQUIRE(sc::findSeamGreedy(energy) == sc::Seam{0, 1, 1});
    REQUIRE(sc::seamCost(energy, sc::findSeamDP(energy)) == 1.0);
}

TEST_CASE("DP and greedy find horizontal seams on the transposed map", "[dp][greedy]") {
    // The cheap horizontal path runs along row 1 then steps down to row 2:
    // rows {1, 1, 2, 2} for columns 0..3, cost 4. carve() finds horizontal
    // seams by transposing, so the vertical seam of energy.t() must be it.
    const cv::Mat energy = (cv::Mat_<double>(3, 4) << 9, 9, 9, 9,
                                                      1, 1, 9, 9,
                                                      9, 9, 1, 1);
    const cv::Mat transposed = energy.t();
    for (const sc::Seam& seam : {sc::findSeamDP(transposed), sc::findSeamGreedy(transposed)}) {
        REQUIRE(seam == sc::Seam{1, 1, 2, 2});
        REQUIRE(sc::seamCost(transposed, seam) == 4.0);
    }
}

TEST_CASE("DP and greedy carve both width and height", "[dp][greedy]") {
    const cv::Mat img = patternImage(); // 9 wide, 7 tall
    for (sc::Method m : {sc::Method::DP, sc::Method::Greedy}) {
        const cv::Mat narrower = sc::carve(img, 6, 7, m);
        REQUIRE(narrower.cols == 6);
        REQUIRE(narrower.rows == 7);
        const cv::Mat shorter = sc::carve(img, 9, 4, m);
        REQUIRE(shorter.cols == 9);
        REQUIRE(shorter.rows == 4);
        const cv::Mat both = sc::carve(img, 5, 5, m);
        REQUIRE(both.cols == 5);
        REQUIRE(both.rows == 5);
        REQUIRE(both.type() == img.type());
    }
}

// Step-by-step visualisation: carve() reports every seam before removing it.

TEST_CASE("carve reports every seam to the callback in order", "[visualise]") {
    const cv::Mat img = patternImage(); // 9 wide, 7 tall
    int vertical = 0;
    int horizontal = 0;
    int expectedCols = img.cols;
    int expectedRows = img.rows;
    const sc::SeamCallback onSeam = [&](const cv::Mat& current, const sc::Seam& seam,
                                        bool isHorizontal) {
        // The image is always upright and shrinks by one each step.
        REQUIRE(current.cols == expectedCols);
        REQUIRE(current.rows == expectedRows);
        if (isHorizontal) {
            REQUIRE(vertical == 3); // all vertical seams come first
            REQUIRE(sc::isValidSeam(seam, current.cols, current.rows));
            ++horizontal;
            --expectedRows;
        } else {
            REQUIRE(horizontal == 0);
            REQUIRE(sc::isValidSeam(seam, current.rows, current.cols));
            ++vertical;
            --expectedCols;
        }
        return true;
    };
    const cv::Mat out = sc::carve(img, 6, 5, sc::Method::DP, onSeam);
    REQUIRE(vertical == 3);
    REQUIRE(horizontal == 2);
    REQUIRE(out.cols == 6);
    REQUIRE(out.rows == 5);
}

TEST_CASE("the callback sees the same seam the method finds", "[visualise]") {
    const cv::Mat img = patternImage();
    bool first = true;
    const sc::SeamCallback onSeam = [&](const cv::Mat& current, const sc::Seam& seam, bool) {
        if (first) REQUIRE(seam == sc::findSeamDP(sc::computeEnergy(current)));
        first = false;
        return true;
    };
    sc::carve(img, 8, 7, sc::Method::DP, onSeam);
    REQUIRE_FALSE(first);
}

TEST_CASE("carve with a callback gives the same result as without", "[visualise]") {
    const cv::Mat img = patternImage();
    const sc::SeamCallback ignore = [](const cv::Mat&, const sc::Seam&, bool) { return true; };
    for (sc::Method m : {sc::Method::DP, sc::Method::Greedy}) {
        const cv::Mat plain = sc::carve(img, 6, 5, m);
        const cv::Mat watched = sc::carve(img, 6, 5, m, ignore);
        REQUIRE(cv::norm(plain, watched, cv::NORM_INF) == 0.0);
    }
}

TEST_CASE("returning false stops the callback but carving still finishes", "[visualise]") {
    const cv::Mat img = patternImage();
    int calls = 0;
    const sc::SeamCallback stopAfterTwo = [&](const cv::Mat&, const sc::Seam&, bool) {
        ++calls;
        return calls < 2;
    };
    const cv::Mat out = sc::carve(img, 5, 4, sc::Method::Greedy, stopAfterTwo);
    REQUIRE(calls == 2);
    REQUIRE(out.cols == 5);
    REQUIRE(out.rows == 4);
}

TEST_CASE("drawHorizontalSeam paints one pixel per column", "[visualise]") {
    const cv::Mat img(3, 4, CV_8UC3, cv::Scalar(10, 20, 30));
    const cv::Mat out = sc::drawHorizontalSeam(img, {0, 1, 2, 2});
    REQUIRE(out.at<cv::Vec3b>(0, 0) == cv::Vec3b(0, 0, 255));
    REQUIRE(out.at<cv::Vec3b>(1, 1) == cv::Vec3b(0, 0, 255));
    REQUIRE(out.at<cv::Vec3b>(2, 2) == cv::Vec3b(0, 0, 255));
    REQUIRE(out.at<cv::Vec3b>(2, 3) == cv::Vec3b(0, 0, 255));
    REQUIRE(out.at<cv::Vec3b>(0, 1) == cv::Vec3b(10, 20, 30));
    REQUIRE(img.at<cv::Vec3b>(0, 0) == cv::Vec3b(10, 20, 30)); // input untouched
    REQUIRE_THROWS_AS(sc::drawHorizontalSeam(img, {0, 1, 2}), cv::Exception); // too short
}

TEST_CASE("renderSeamStep draws a visible red seam for both directions", "[visualise]") {
    const cv::Mat img(30, 40, CV_8UC3, cv::Scalar(200, 200, 200));
    const auto hasRed = [](const cv::Mat& canvas) {
        for (int r = 0; r < canvas.rows; ++r)
            for (int c = 0; c < canvas.cols; ++c)
                if (canvas.at<cv::Vec3b>(r, c) == cv::Vec3b(0, 0, 255)) return true;
        return false;
    };
    const cv::Mat v = sc::renderSeamStep(img, sc::Seam(30, 5), false, "vertical");
    const cv::Mat h = sc::renderSeamStep(img, sc::Seam(40, 7), true, "horizontal");
    for (const cv::Mat* canvas : {&v, &h}) {
        REQUIRE(canvas->type() == CV_8UC3);
        REQUIRE(canvas->cols > img.cols);
        REQUIRE(canvas->rows > img.rows);
        REQUIRE(hasRed(*canvas));
    }
    REQUIRE_THROWS_AS(sc::renderSeamStep(img, sc::Seam(40, 7), false, "wrong length"),
                      cv::Exception);
}

TEST_CASE("DP is optimal and greedy never beats it on random maps", "[dp][greedy]") {
    // 50 random 6x7 energy maps with small integer values (lots of ties).
    cv::RNG rng(12345); // fixed seed: the same maps every run
    for (int trial = 0; trial < 50; ++trial) {
        cv::Mat energy(6, 7, CV_64FC1);
        for (int r = 0; r < energy.rows; ++r) {
            for (int c = 0; c < energy.cols; ++c) {
                energy.at<double>(r, c) = rng.uniform(0, 10);
            }
        }
        double best = -1.0;
        for (int c = 0; c < energy.cols; ++c) {
            const double cost = bruteForceMinCost(energy, 0, c);
            if (best < 0.0 || cost < best) best = cost;
        }
        const sc::Seam dp = sc::findSeamDP(energy);
        const sc::Seam greedy = sc::findSeamGreedy(energy);
        REQUIRE(sc::isValidSeam(dp, energy.rows, energy.cols));
        REQUIRE(sc::isValidSeam(greedy, energy.rows, energy.cols));
        REQUIRE(sc::seamCost(energy, dp) == best);
        REQUIRE(sc::seamCost(energy, greedy) >= best);
    }
}
