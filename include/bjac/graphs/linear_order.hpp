#ifndef INCLUDE_BJAC_GRAPHS_LINEAR_ORDER_HPP
#define INCLUDE_BJAC_GRAPHS_LINEAR_ORDER_HPP

#include <cassert>
#include <deque>
#include <ranges>
#include <unordered_map>
#include <vector>

#include "bjac/graphs/dfs.hpp"
#include "bjac/graphs/dominator_tree.hpp"
#include "bjac/graphs/loop_tree.hpp"

namespace bjac {

template <typename Traits>
class LinearOrder final : private std::vector<typename Traits::vertex_handler> {
    enum class Color : bool { white, black };

    using base = std::vector<typename Traits::vertex_handler>;

  public:
    using graph_type = typename Traits::graph_type;
    using vertex_handler = typename Traits::vertex_handler;

    using base::begin;
    using base::cbegin;
    using base::cend;
    using base::crbegin;
    using base::crend;
    using base::end;
    using base::rbegin;
    using base::rend;
    using base::size;

    explicit LinearOrder(graph_type &g) : LinearOrder{g, DFS<Traits>{g}} {}

    explicit LinearOrder(graph_type &g, const DFS<Traits> &dfs)
        : LinearOrder{g, dfs, DominatorTree<Traits>{g, dfs}} {}

    explicit LinearOrder(graph_type &g, const DFS<Traits> &dfs,
                         const DominatorTree<Traits> &dom_tree)
        : LinearOrder{g, dom_tree, LoopTree<Traits>{g, dfs, dom_tree}} {}

    explicit LinearOrder(graph_type &g, const DominatorTree<Traits> &dom_tree,
                         const LoopTree<Traits> &loop_tree) {
        this->reserve(Traits::n_vertices(g));

        auto color_table = [&g] {
            std::unordered_map<vertex_handler, Color> color_table;
            color_table.reserve(Traits::n_vertices(g));
            color_table.emplace(Traits::source(g), Color::black);
            for (auto v : Traits::vertices(g)) {
                color_table.emplace(v, Color::white);
            }
            return color_table;
        }();

        std::deque<vertex_handler> queue;
        queue.push_back(Traits::source(g));

        while (!queue.empty()) {
            const vertex_handler u = queue.front();
            queue.pop_front();

            this->push_back(u);

            if (loop_tree.is_header(u)) {
                // drop(1) not to process the header again
                for (auto v :
                     loop_tree.get_loop(u).vertices() | std::views::drop(1) | std::views::reverse) {
                    auto &color = color_table.find(v)->second;
                    assert(color == Color::white);
                    color = Color::black;
                    queue.push_front(v);
                }
            }

            for (auto v : dom_tree.successors(u)) {
                if (auto &color = color_table.find(v)->second; color == Color::white) {
                    color = Color::black;
                    queue.push_back(v);
                }
            }
        }
    }
};

} // namespace bjac

#endif // INCLUDE_BJAC_GRAPHS_LINEAR_ORDER_HPP
