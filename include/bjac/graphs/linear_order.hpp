#ifndef INCLUDE_BJAC_GRAPHS_LINEAR_ORDER_HPP
#define INCLUDE_BJAC_GRAPHS_LINEAR_ORDER_HPP

#include <queue>
#include <ranges>
#include <vector>

#include "bjac/graphs/dfs.hpp"
#include "bjac/graphs/dominator_tree.hpp"
#include "bjac/graphs/loop_tree.hpp"

namespace bjac {

template <typename Traits>
class LinearOrder final {
  public:
    using graph_type = typename Traits::graph_type;
    using vertex_handler = typename Traits::vertex_handler;

  private:
    enum class Color : bool { white, black };

  public:
    explicit LinearOrder(graph_type &g) : LinearOrder{g, DFS<Traits>{g}} {}

    explicit LinearOrder(graph_type &g, const DFS<Traits> &dfs)
        : LinearOrder{g, dfs, DominatorTree<Traits>{g, dfs}} {}

    explicit LinearOrder(graph_type &g, const DFS<Traits> &dfs,
                         const DominatorTree<Traits> &dom_tree)
        : LinearOrder{g, dom_tree, LoopTree<Traits>{g, dfs, dom_tree}} {}

    explicit LinearOrder(graph_type &g, const DominatorTree<Traits> &dom_tree,
                         const LoopTree<Traits> &loop_tree) {
        order_.reserve(Traits::n_vertices(g));

        auto color_table = [&g] {
            std::unordered_map<vertex_handler, Color> color_table;
            color_table.reserve(Traits::n_vertices(g));
            color_table.emplace(Traits::source(g), Color::black);
            for (auto v : Traits::vertices(g)) {
                color_table.emplace(v, Color::white);
            }
            return color_table;
        }();

        std::queue<vertex_handler> queue;
        queue.push(Traits::source(g));

        while (!queue.empty()) {
            const vertex_handler u = queue.front();
            queue.pop();

            order_.push_back(u);

            if (loop_tree.is_header(u)) {
                for (auto v : loop_tree.get_loop(u).vertices()) {
                    if (auto &color = color_table.find(v)->second;
                        color == Color::white /* also implies that v != u*/) {
                        color = Color::black;
                        queue.push(v);
                    }
                }
            }

            for (auto v : dom_tree.successors(u)) {
                if (auto &color = color_table.find(v)->second; color == Color::white) {
                    color = Color::black;
                    queue.push(v);
                }
            }
        }
    }

    std::ranges::random_access_range auto blocks() const { return std::views::all(order_); }

  private:
    std::vector<vertex_handler> order_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_GRAPHS_LINEAR_ORDER_HPP
