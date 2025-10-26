#ifndef INCLUDE_BJAC_GRAPHS_LOOP_TREE_HPP
#define INCLUDE_BJAC_GRAPHS_LOOP_TREE_HPP

#include <concepts>
#include <memory>
#include <ranges>
#include <unordered_map>
#include <utility>
#include <vector>

#include "bjac/graphs//inverse_graph_adaptor.hpp"
#include "bjac/graphs/dominator_tree.hpp"
#include "bjac/graphs/loop.hpp"

namespace bjac {

template <typename G, typename Traits>
class LoopTree final {
    using vertex_handler = typename Traits::vertex_handler;

  public:
    explicit LoopTree(const G &g) {
        using InverseGraph = InverseGraphAdaptor<G, Traits>;
        const InverseGraph inverse_graph{g};

        for (const auto &[latch, header] : compute_back_edges(g)) {
            DFS<InverseGraph, InverseGraphAdaptorTraits<G, Traits>> dfs(inverse_graph, latch,
                                                                        {header});

            auto loop = std::make_unique<Loop<vertex_handler>>(header);

            loop->add_vertex(header);
            for (vertex_handler v : dfs.post_order()) {
                loop->add_vertex(v);
                if (auto it = header_to_loop_.find(v); it != header_to_loop_.end()) {
                    loop->add_inner_loop(std::move(it->second));
                    header_to_loop_.erase(it);
                }
            }

            header_to_loop_.emplace(header, std::move(loop));
        }
    }

    std::unsigned_integral auto loops_count() const noexcept { return header_to_loop_.size(); }
    template<typename Self>
    auto &get_loop(this Self &&self, vertex_handler header) {
        return std::forward_like<Self>(*self.header_to_loop_.at(header));
    }
    std::ranges::forward_range auto loops() const {
        return header_to_loop_ | std::views::values |
               std::views::transform([](const auto &loop_ptr) static { return loop_ptr.get(); });
    }

  private:
    static auto compute_back_edges(const G &g) {
        const DFS<G, Traits> dfs{g, Traits::source(g)};
        const DominatorTree<G, Traits> dom_tree{g, dfs};

        std::vector<std::pair<vertex_handler, vertex_handler>> back_edges_;
        for (vertex_handler v : dfs.pre_order()) {
            for (vertex_handler u : Traits::adjacent_vertices(g, v)) {
                if (dom_tree.is_dominator_of(v, u)) {
                    back_edges_.emplace_back(v, u);
                }
            }
        }

        return back_edges_;
    }

    std::unordered_map<vertex_handler, std::unique_ptr<Loop<vertex_handler>>> header_to_loop_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_GRAPHS_LOOP_TREE_HPP
