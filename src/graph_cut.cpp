#include "seam_carving/graph_cut.hpp"

#include <algorithm>
#include <limits>
#include <queue>
#include <vector>

namespace sc {

namespace {

constexpr double kInf = std::numeric_limits<double>::infinity();

// Residual capacities below this are treated as zero (guards against
// floating-point leftovers after subtracting flows).
constexpr double kEps = 1e-9;

// Directed graph stored as an edge list plus per-node adjacency lists.
// Every edge is added together with its reverse edge (capacity 0), so the
// residual graph is just the same arrays with `cap` updated in place.
struct FlowGraph {
    struct Edge {
        int to;
        double cap; // residual capacity
    };

    std::vector<Edge> edges;            // edge e and its reverse are e and e ^ 1
    std::vector<std::vector<int>> adj;  // edge ids leaving each node

    explicit FlowGraph(int nodes) : adj(static_cast<size_t>(nodes)) {}

    void addEdge(int from, int to, double cap) {
        adj[static_cast<size_t>(from)].push_back(static_cast<int>(edges.size()));
        edges.push_back({to, cap});
        adj[static_cast<size_t>(to)].push_back(static_cast<int>(edges.size()));
        edges.push_back({from, 0.0});
    }

    // Edmonds-Karp: repeatedly find the shortest (fewest edges) path from
    // source to sink with BFS in the residual graph and push the bottleneck
    // capacity along it, until no such path exists.
    void maxFlow(int source, int sink) {
        const size_t n = adj.size();
        std::vector<int> parentEdge(n);
        std::queue<int> queue;
        for (;;) {
            // BFS from the source; parentEdge[v] is the edge used to reach v.
            std::fill(parentEdge.begin(), parentEdge.end(), -1);
            parentEdge[static_cast<size_t>(source)] = -2;
            queue.push(source);
            while (!queue.empty() && parentEdge[static_cast<size_t>(sink)] == -1) {
                const int u = queue.front();
                queue.pop();
                for (int id : adj[static_cast<size_t>(u)]) {
                    const Edge& e = edges[static_cast<size_t>(id)];
                    if (e.cap > kEps && parentEdge[static_cast<size_t>(e.to)] == -1) {
                        parentEdge[static_cast<size_t>(e.to)] = id;
                        queue.push(e.to);
                    }
                }
            }
            queue = {};
            if (parentEdge[static_cast<size_t>(sink)] == -1) {
                return; // no augmenting path left: the flow is maximum
            }

            // Bottleneck = smallest residual capacity on the path.
            double bottleneck = kInf;
            for (int v = sink; v != source;) {
                const int id = parentEdge[static_cast<size_t>(v)];
                bottleneck = std::min(bottleneck, edges[static_cast<size_t>(id)].cap);
                v = edges[static_cast<size_t>(id ^ 1)].to;
            }

            // Push it: forward residuals shrink, reverse residuals grow.
            for (int v = sink; v != source;) {
                const int id = parentEdge[static_cast<size_t>(v)];
                edges[static_cast<size_t>(id)].cap -= bottleneck;
                edges[static_cast<size_t>(id ^ 1)].cap += bottleneck;
                v = edges[static_cast<size_t>(id ^ 1)].to;
            }
        }
    }

    // Nodes reachable from `source` through edges with residual capacity.
    // After maxFlow this is the source side of a minimum cut.
    std::vector<char> reachableFrom(int source) const {
        std::vector<char> seen(adj.size(), 0);
        std::queue<int> queue;
        seen[static_cast<size_t>(source)] = 1;
        queue.push(source);
        while (!queue.empty()) {
            const int u = queue.front();
            queue.pop();
            for (int id : adj[static_cast<size_t>(u)]) {
                const Edge& e = edges[static_cast<size_t>(id)];
                if (e.cap > kEps && !seen[static_cast<size_t>(e.to)]) {
                    seen[static_cast<size_t>(e.to)] = 1;
                    queue.push(e.to);
                }
            }
        }
        return seen;
    }
};

} // namespace

Seam findSeamGraphCut(const cv::Mat& energy) {
    // Step 1: validate input. computeEnergy() always gives CV_64FC1.
    CV_Assert(!energy.empty());
    CV_Assert(energy.type() == CV_64FC1);

    const int rows = energy.rows;
    const int cols = energy.cols;

    // Step 2: one node per pixel, plus a source S and a sink T.
    // Pixel (r, c) is node r * cols + c.
    const int source = rows * cols;
    const int sink = rows * cols + 1;
    auto node = [cols](int r, int c) { return r * cols + c; };
    FlowGraph graph(rows * cols + 2);

    // Idea: a cut splits each row into a left part (source side) and a right
    // part (sink side). The seam pixel of row r is the LAST source-side pixel,
    // and cutting through it costs its energy. Infinite edges cannot be cut,
    // so they are used to force the cut to have the shape of a valid seam.
    for (int r = 0; r < rows; ++r) {
        // Step 3: S -> first column, infinite. The left edge is always on the
        // source side, so every row has at least one source-side pixel.
        graph.addEdge(source, node(r, 0), kInf);

        for (int c = 0; c < cols; ++c) {
            const double e = energy.at<double>(r, c);

            // Step 4: forward edge with capacity e[r][c]. Cutting it means
            // pixel (r, c) is the seam pixel of row r. For the last column
            // the "next pixel" is the sink itself, so a seam can also run
            // through the last column.
            if (c + 1 < cols) {
                graph.addEdge(node(r, c), node(r, c + 1), e);
                // Step 5: infinite backward edge. If (r, c+1) is on the source
                // side, (r, c) must be too, so each row is cut exactly once.
                graph.addEdge(node(r, c + 1), node(r, c), kInf);
            } else {
                graph.addEdge(node(r, c), sink, e);
            }

            // Step 6: infinite diagonal edges back to the left, to the rows
            // above and below. If (r, c) is on the source side, so are
            // (r-1, c-1) and (r+1, c-1). This makes seam columns in adjacent
            // rows differ by at most one, i.e. the seam is connected.
            if (c > 0) {
                if (r > 0) graph.addEdge(node(r, c), node(r - 1, c - 1), kInf);
                if (r + 1 < rows) graph.addEdge(node(r, c), node(r + 1, c - 1), kInf);
            }
        }
    }
    // Every S -> T path must cross a finite forward edge (all the infinite
    // edges point left or come from S), so the max flow is finite.

    // Step 7: max flow = min cut (max-flow min-cut theorem).
    graph.maxFlow(source, sink);

    // Step 8: the source side of the min cut is everything still reachable
    // from S in the residual graph. In each row it is a prefix of columns
    // (because of the backward edges); its rightmost pixel is the seam pixel.
    const std::vector<char> sourceSide = graph.reachableFrom(source);
    Seam seam(rows);
    for (int r = 0; r < rows; ++r) {
        int last = 0;
        while (last + 1 < cols && sourceSide[static_cast<size_t>(node(r, last + 1))]) {
            ++last;
        }
        seam[r] = last;
    }

    // Size: V = rows*cols + 2 nodes and about 4*rows*cols edges.
    // Edmonds-Karp is O(V * E^2) in the worst case, so this is far slower
    // than DP; use small images (e.g. images/input/sample.png).
    return seam;
}

} // namespace sc
