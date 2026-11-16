#include "seam_carving/seam.hpp"

#include <cstdlib>
#include <stdexcept>

#include "seam_carving/dp.hpp"
#include "seam_carving/graph_cut.hpp"
#include "seam_carving/greedy.hpp"

namespace sc {

Seam findSeam(const cv::Mat& energy, Method method) {
    switch (method) {
        case Method::DP:       return findSeamDP(energy);
        case Method::Greedy:   return findSeamGreedy(energy);
        case Method::GraphCut: return findSeamGraphCut(energy);
    }
    throw std::invalid_argument("unknown method");
}

bool isValidSeam(const Seam& seam, int rows, int cols) {
    if (static_cast<int>(seam.size()) != rows) return false;
    for (int r = 0; r < rows; ++r) {
        if (seam[r] < 0 || seam[r] >= cols) return false;
        if (r > 0 && std::abs(seam[r] - seam[r - 1]) > 1) return false;
    }
    return true;
}

double seamCost(const cv::Mat& energy, const Seam& seam) {
    CV_Assert(energy.type() == CV_64FC1);
    CV_Assert(isValidSeam(seam, energy.rows, energy.cols));
    double total = 0.0;
    for (int r = 0; r < energy.rows; ++r) total += energy.at<double>(r, seam[r]);
    return total;
}

Method parseMethod(const std::string& name) {
    if (name == "dp") return Method::DP;
    if (name == "greedy") return Method::Greedy;
    if (name == "graphcut") return Method::GraphCut;
    throw std::invalid_argument("unknown method '" + name + "' (use dp, greedy or graphcut)");
}

std::string methodName(Method method) {
    switch (method) {
        case Method::DP:       return "dp";
        case Method::Greedy:   return "greedy";
        case Method::GraphCut: return "graphcut";
    }
    return "unknown";
}

} // namespace sc
