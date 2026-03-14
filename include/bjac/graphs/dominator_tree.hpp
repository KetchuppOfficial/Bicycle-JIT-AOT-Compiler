#ifndef INCLUDE_BJAC_GRAPHS_DOMINATOR_TREE_HPP
#define INCLUDE_BJAC_GRAPHS_DOMINATOR_TREE_HPP

#include <algorithm>
#include <cassert>
#include <optional>
#include <ranges>
#include <unordered_map>

#include "bjac/graphs/dfs.hpp"

namespace bjac {

template <typename Traits>
class DominatorTree final {
  public:
    using graph_type = typename Traits::graph_type;
    using vertex_handler = typename Traits::vertex_handler;

    struct Edges {
        std::optional<vertex_handler> idom;
        std::vector<vertex_handler> successors;
    };

  private:
    using VertexToIDomContainer = std::unordered_map<vertex_handler, Edges>;

  public:
    using const_iterator = typename VertexToIDomContainer::const_iterator;
    using iterator = const_iterator;

    explicit DominatorTree(graph_type &g) : DominatorTree{g, DFS<Traits>{g}} {}

    explicit DominatorTree(graph_type &g, const DFS<Traits> &dfs) {
        compute_idoms(dfs, compute_semidominators(g, dfs));

        v_to_idom_.try_emplace(Traits::source(g));

        // Note: successors are stored in reverse post order. This makes it possible to traverse
        // dominator tree in BFS and yield RPO as a result
        for (auto v : dfs.post_order() | std::views::reverse) {
            auto v_it = v_to_idom_.find(v);
            assert(v_it != v_to_idom_.end());
            const auto &maybe_idom = v_it->second.idom;
            if (maybe_idom) {
                auto idom_it = v_to_idom_.find(*maybe_idom);
                assert(idom_it != v_to_idom_.end());
                idom_it->second.successors.push_back(v);
            }
        }
    }

    bool contains(vertex_handler v) const { return v_to_idom_.contains(v); }

    vertex_handler idom_unchecked(vertex_handler v) const {
        assert(contains(v));
        return *v_to_idom_.find(v)->second.idom;
    }

    std::optional<vertex_handler> idom(vertex_handler v) const {
        if (auto it = v_to_idom_.find(v); it != v_to_idom_.end()) {
            return it->second.idom;
        }
        return std::nullopt;
    }

    bool is_dominator_of(vertex_handler v, vertex_handler u) const {
        for (;;) {
            if (auto v_it = v_to_idom_.find(v); v_it == v_to_idom_.end()) {
                return false;
            } else if (const auto &idom = v_it->second.idom; !idom.has_value()) {
                return false;
            } else if (*idom == u) {
                return true;
            } else {
                v = *idom;
            }
        }
    }

    std::ranges::random_access_range auto successors(vertex_handler v) const {
        return std::views::all(v_to_idom_.at(v).successors);
    }

    const_iterator begin() const { return v_to_idom_.begin(); }
    const_iterator cbegin() const { return v_to_idom_.cbegin(); }
    const_iterator end() const { return v_to_idom_.end(); }
    const_iterator cend() const { return v_to_idom_.cend(); }

  private:
    using time_type = typename DFS<Traits>::time_type;

    struct SDom {
        vertex_handler vertex;
        time_type time;
    };

    using VertexToSDomContainer = std::unordered_map<vertex_handler, SDom>;

    static VertexToSDomContainer compute_semidominators(const graph_type &g,
                                                        const DFS<Traits> &dfs) {
        auto discovery_time = [&dfs](vertex_handler v) { return dfs.info(v).get_discovery_time(); };

        VertexToSDomContainer v_to_sdom;
        v_to_sdom.reserve(dfs.size());

        std::vector<vertex_handler> visited_vertices;
        visited_vertices.reserve(dfs.size());

        for (vertex_handler w : dfs.pre_order() | std::views::drop(1) | std::views::reverse) {
            auto preds = Traits::predecessors(g, w);

            auto sdom_candidates_1 =
                preds |
                std::views::filter([discovery_time, w_time = discovery_time(w)](vertex_handler v) {
                    return discovery_time(v) < w_time;
                });

            auto sdom_candidates_2 =
                visited_vertices | std::views::filter([&](vertex_handler u) {
                    return std::ranges::any_of(
                        preds, [&dfs, u](vertex_handler v) { return dfs.is_ancestor_of(v, u); });
                }) |
                std::views::transform([&v_to_sdom](vertex_handler u) {
                    auto it = v_to_sdom.find(u);
                    assert(it != v_to_sdom.end());
                    return it->second.vertex;
                });

            vertex_handler sdom = std::ranges::min(
                std::views::concat(sdom_candidates_1, sdom_candidates_2), {}, discovery_time);

            v_to_sdom.try_emplace(w, sdom, discovery_time(sdom));
            visited_vertices.push_back(w);
        }

        return v_to_sdom;
    }

    void compute_idoms(const DFS<Traits> &dfs, const VertexToSDomContainer &v_to_sdom) {
        v_to_idom_.reserve(dfs.size());

        auto pre_order = dfs.pre_order() | std::views::drop(1);        // drop entry block
        static_assert(std::ranges::common_range<decltype(pre_order)>); // to guarantee O(1) reverse

        for (vertex_handler w : pre_order | std::views::reverse) {
            auto w_it = v_to_sdom.find(w);
            assert(w_it != v_to_sdom.end());
            vertex_handler sdom_w = w_it->second.vertex;
            auto it = std::ranges::min_element(dfs.st_begin(w), dfs.st_begin(sdom_w), {},
                                               [&](const auto &info) {
                                                   auto it = v_to_sdom.find(info.get_vertex());
                                                   assert(it != v_to_sdom.end());
                                                   return it->second.time;
                                               });
            vertex_handler u = it->get_vertex();
            auto u_it = v_to_sdom.find(u);
            assert(u_it != v_to_sdom.end());
            if (vertex_handler sdom_u = u_it->second.vertex; sdom_u == sdom_w) {
                v_to_idom_.emplace(w, sdom_w);
            } else {
                v_to_idom_.emplace(w, u);
            }
        }

        for (vertex_handler w : pre_order) {
            auto w_it = v_to_idom_.find(w);
            assert(w_it != v_to_idom_.end());
            auto &maybe_idom = w_it->second.idom;
            assert(maybe_idom.has_value());
            vertex_handler idom = *maybe_idom;
            if (idom != v_to_sdom.at(w).vertex) {
                auto idom_it = v_to_idom_.find(idom);
                assert(idom_it != v_to_idom_.end());
                maybe_idom = idom_it->second.idom;
            }
        }
    }

    VertexToIDomContainer v_to_idom_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_GRAPHS_DOMINATOR_TREE_HPP
