#pragma once
#include <opencv2/core.hpp>
#include <string>
#include <vector>

namespace sc {

// A vertical seam: seam[r] is the column of the pixel removed from row r.
// Horizontal seams are handled by transposing the image (see carve.cpp).
using Seam = std::vector<int>;

enum class Method { DP, Greedy, GraphCut };

// Dispatches to findSeamDP / findSeamGreedy / findSeamGraphCut.
// `energy` is a CV_64FC1 map from computeEnergy().
Seam findSeam(const cv::Mat& energy, Method method);

// True if the seam has one in-range column per row and adjacent rows
// differ by at most one column.
bool isValidSeam(const Seam& seam, int rows, int cols);

// Sum of the energy along the seam.
double seamCost(const cv::Mat& energy, const Seam& seam);

// "dp" | "greedy" | "graphcut"  ->  Method. Throws std::invalid_argument.
Method parseMethod(const std::string& name);
std::string methodName(Method method);

} // namespace sc
