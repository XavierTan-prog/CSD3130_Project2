#include "seam_carving/graph_cut.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace sc {
namespace {

constexpr double kUnboundedCapacity = std::numeric_limits<double>::infinity();

struct FlowEdge {
    int to;
    int reverseIndex;
    double residual;
};

class FlowNetwork {
public:
    FlowNetwork(int width, int height) : edges_(width * height + 2) {
        for (int row = 0; row < height; ++row) {
            const int adjacentRows = (row > 0) + (row + 1 < height);
            for (int col = 0; col < width; ++col) {
                const int adjacentCols = (col > 0) + (col + 1 < width);
                // Two energy/terminal entries, plus a backward edge and the
                // diagonal constraints (or their reverse entries) on each side.
                edges_[row * width + col].reserve(2 + adjacentCols * (1 + adjacentRows));
            }
        }
        const int source = width * height;
        edges_[source].reserve(height);
        edges_[source + 1].reserve(height);
    }

    void addEdge(int from, int to, double capacity) {
        const int forwardIndex = static_cast<int>(edges_[from].size());
        const int reverseIndex = static_cast<int>(edges_[to].size());
        edges_[from].push_back({to, reverseIndex, capacity});
        edges_[to].push_back({from, forwardIndex, 0.0});
    }

    std::vector<bool> minCutSourceSide(int source, int sink) {
        std::vector<int> level(edges_.size());
        std::vector<int> nextEdge(edges_.size());
        std::vector<int> queue;
        std::vector<int> path;
        queue.reserve(edges_.size());
        path.reserve(edges_.size());

        // Dinic sends a blocking flow through the BFS levels before rebuilding them.
        while (buildLevels(source, sink, level, queue)) {
            sendBlockingFlow(source, sink, level, nextEdge, path);
        }

        // The sink is unreachable. Find the source side of the final cut.
        std::vector<bool> sourceSide(edges_.size());
        queue.clear();
        queue.push_back(source);
        sourceSide[source] = true;
        for (std::size_t head = 0; head < queue.size(); ++head) {
            for (const FlowEdge& edge : edges_[queue[head]]) {
                if (edge.residual <= 0.0 || sourceSide[edge.to]) continue;
                sourceSide[edge.to] = true;
                queue.push_back(edge.to);
            }
        }
        return sourceSide;
    }

private:
    bool buildLevels(int source, int sink, std::vector<int>& level,
                     std::vector<int>& queue) const {
        std::fill(level.begin(), level.end(), -1);
        level[sink] = 0;
        queue.clear();
        queue.push_back(sink);

        // Label distances to the sink by following incoming residual edges.
        for (std::size_t head = 0; head < queue.size(); ++head) {
            const int from = queue[head];
            for (const FlowEdge& edge : edges_[from]) {
                if (level[edge.to] != -1) continue;
                const FlowEdge& incoming = edges_[edge.to][edge.reverseIndex];
                if (incoming.residual <= 0.0) continue;
                level[edge.to] = level[from] + 1;
                // All vertices closer to the sink than the source have a level.
                if (edge.to == source) return true;
                queue.push_back(edge.to);
            }
        }
        return false;
    }

    void sendBlockingFlow(int source, int sink, const std::vector<int>& level,
                          std::vector<int>& nextEdge, std::vector<int>& path) {
        std::fill(nextEdge.begin(), nextEdge.end(), 0);
        path.clear();
        path.push_back(source);

        // Iterate over the level graph with an explicit stack for deep paths.
        while (!path.empty()) {
            const int from = path.back();
            if (from == sink) {
                path.resize(augmentPath(path, nextEdge));
                continue;
            }

            int& next = nextEdge[from];
            const auto& outgoing = edges_[from];
            const int edgeCount = static_cast<int>(outgoing.size());
            const int nextLevel = level[from] - 1;
            for (; next < edgeCount; ++next) {
                const FlowEdge& edge = outgoing[next];
                if (edge.residual > 0.0 && level[edge.to] == nextLevel) break;
            }

            if (next < edgeCount) {
                path.push_back(outgoing[next].to);
            } else {
                // Reverse updates lead away from the sink. This dead end stays closed
                // for the rest of the phase.
                path.pop_back();
                if (!path.empty()) ++nextEdge[path.back()];
            }
        }
    }

    std::size_t augmentPath(const std::vector<int>& path, const std::vector<int>& nextEdge) {
        double bottleneck = kUnboundedCapacity;
        std::size_t prefixLength = 1;
        for (std::size_t i = 0; i + 1 < path.size(); ++i) {
            const int from = path[i];
            const double residual = edges_[from][nextEdge[from]].residual;
            if (residual < bottleneck) {
                bottleneck = residual;
                prefixLength = i + 1;
            }
        }
        // Every path into the sink crosses a finite pixel-energy edge.
        CV_Assert(std::isfinite(bottleneck));

        for (std::size_t i = 0; i + 1 < path.size(); ++i) {
            const int from = path[i];
            FlowEdge& edge = edges_[from][nextEdge[from]];
            FlowEdge& reverse = edges_[edge.to][edge.reverseIndex];
            const double reverseResidual = reverse.residual + bottleneck;
            CV_Assert(std::isfinite(reverseResidual) || reverse.residual == kUnboundedCapacity);
            edge.residual -= bottleneck;
            reverse.residual = reverseResidual;
        }
        // Keep the prefix ending just before the first saturated edge.
        return prefixLength;
    }

    std::vector<std::vector<FlowEdge>> edges_;
};

} // namespace

Seam findSeamGraphCut(const cv::Mat& energy) {
    CV_Assert(!energy.empty() && energy.dims == 2 && energy.type() == CV_64FC1);
    CV_Assert(energy.total() <= static_cast<std::size_t>(std::numeric_limits<int>::max() - 2));

    const int width = energy.cols;
    const int height = energy.rows;
    const auto pixel = [width](int row, int col) { return row * width + col; };
    const int source = static_cast<int>(energy.total());
    const int sink = source + 1;
    FlowNetwork network(width, height);

    for (int row = 0; row < height; ++row) {
        network.addEdge(source, pixel(row, 0), kUnboundedCapacity);
        const double* rowEnergy = energy.ptr<double>(row);
        for (int col = 0; col < width; ++col) {
            const int vertex = pixel(row, col);
            const double cost = rowEnergy[col];
            CV_Assert(std::isfinite(cost) && cost >= 0.0);

            // The last pixel's energy edge goes to the sink, letting the cut
            // select that pixel even when it is the only column.
            network.addEdge(vertex, col + 1 < width ? vertex + 1 : sink, cost);

            if (col == 0) continue;
            // Backward edges keep each source-side row a contiguous prefix.
            network.addEdge(vertex, pixel(row, col - 1), kUnboundedCapacity);
            // Backward diagonals force neighboring rows to reach col - 1.
            if (row > 0) {
                network.addEdge(vertex, pixel(row - 1, col - 1), kUnboundedCapacity);
            }
            if (row + 1 < height) {
                network.addEdge(vertex, pixel(row + 1, col - 1), kUnboundedCapacity);
            }
        }
    }

    const auto sourceSide = network.minCutSourceSide(source, sink);
    Seam seam(height, -1);
    // Each row pays for the outgoing energy edge at its last source-side pixel.
    for (int row = 0; row < height; ++row) {
        for (int col = width - 1; col >= 0; --col) {
            if (sourceSide[pixel(row, col)]) {
                seam[row] = col;
                break;
            }
        }
    }
    CV_Assert(isValidSeam(seam, height, width));
    return seam;
}

} // namespace sc
