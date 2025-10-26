#ifndef INCLUDE_BJAC_GRAPHS_DOMINATOR_TREE_HPP
#define INCLUDE_BJAC_GRAPHS_DOMINATOR_TREE_HPP

#include <algorithm>
#include <cassert>
#include <optional>
#include <ranges>
#include <unordered_map>

#include "bjac/graphs/dfs.hpp"

namespace bjac {

template <typename G, typename Traits>
class DominatorTree final {
  public:
    using vertex_handler = typename Traits::vertex_handler;

  private:
    using VertexToIDomContainer = std::unordered_map<vertex_handler, vertex_handler>;

  public:
    using const_iterator = typename VertexToIDomContainer::const_iterator;
    using iterator = const_iterator;

    explicit DominatorTree(const G &g) : DominatorTree{g, DFS<G, Traits>{g, Traits::source(g)}} {}

    explicit DominatorTree(const G &g, const DFS<G, Traits> &dfs) {
        compute_idoms(dfs, compute_semidominators(g, dfs));
    }

    bool contains(vertex_handler v) const { return v_to_idom_.contains(v); }

    vertex_handler idom_unchecked(vertex_handler v) const {
        assert(contains(v));
        return v_to_idom_.find(v)->second;
    }

    std::optional<vertex_handler> idom(vertex_handler v) const {
        if (auto it = v_to_idom_.find(v); it != v_to_idom_.end()) {
            return {it->second};
        }
        return std::nullopt;
    }

    bool is_dominator_of(vertex_handler v, vertex_handler u) const {
        for (;;) {
            if (auto v_it = v_to_idom_.find(v); v_it == v_to_idom_.end()) {
                return false;
            } else {
                if (vertex_handler idom = v_it->second; idom == u) {
                    return true;
                } else {
                    v = idom;
                }
            }
        }
    }

    const_iterator begin() const { return v_to_idom_.begin(); }
    const_iterator cbegin() const { return v_to_idom_.cbegin(); }
    const_iterator end() const { return v_to_idom_.end(); }
    const_iterator cend() const { return v_to_idom_.cend(); }

  private:
    using time_type = typename DFS<G, Traits>::time_type;

    struct SDom {
        vertex_handler vertex;
        time_type time;
    };

    using VertexToSDomContainer = std::unordered_map<vertex_handler, SDom>;

    static VertexToSDomContainer compute_semidominators(const G &g, const DFS<G, Traits> &dfs) {
        auto comp = [&dfs](vertex_handler v, vertex_handler u) {
            return dfs.info(v).get_discovery_time() < dfs.info(u).get_discovery_time();
        };

        VertexToSDomContainer v_to_sdom;
        v_to_sdom.reserve(dfs.size());

        std::vector<vertex_handler> visited_vertices;
        visited_vertices.reserve(dfs.size());

        for (vertex_handler w :
             dfs.pre_order() | std::views::reverse | std::views::take(dfs.size() - 1)) {
            auto preds = Traits::predecessors(g, w);

            auto sdom_candidates_1 = std::views::filter(preds, [&dfs, w](vertex_handler v) {
                return dfs.info(v).get_discovery_time() < dfs.info(w).get_discovery_time();
            });

            auto sdom_candidates_2 =
                visited_vertices | std::views::filter([&](vertex_handler u) {
                    return std::ranges::any_of(
                        preds, [&dfs, u](vertex_handler v) { return dfs.is_ancestor_of(v, u); });
                }) |
                std::views::transform([&v_to_sdom = v_to_sdom](vertex_handler u) {
                    auto it = v_to_sdom.find(u);
                    assert(it != v_to_sdom.end());
                    return it->second.vertex;
                });

            auto sdom_candidates = std::views::concat(sdom_candidates_1, sdom_candidates_2);
            vertex_handler sdom = *std::ranges::min_element(sdom_candidates, comp);

            v_to_sdom.try_emplace(w, sdom, dfs.info(sdom).get_discovery_time());
            visited_vertices.push_back(w);
        }

        return v_to_sdom;
    }

    void compute_idoms(const DFS<G, Traits> &dfs, const VertexToSDomContainer &v_to_sdom) {
        v_to_idom_.reserve(dfs.size());

        auto pre_order = dfs.pre_order();

        for (vertex_handler w :
             pre_order | std::views::reverse | std::views::take(dfs.size() - 1)) {
            auto w_it = v_to_sdom.find(w);
            assert(w_it != v_to_sdom.end());
            vertex_handler sdom_w = w_it->second.vertex;
            auto it = std::ranges::min_element(dfs.st_begin(w), dfs.st_begin(sdom_w),
                                               std::ranges::less{}, [&](const auto &info) {
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

        for (vertex_handler w : pre_order | std::views::drop(1)) {
            auto w_it = v_to_idom_.find(w);
            assert(w_it != v_to_idom_.end());
            if (vertex_handler &idom = w_it->second; idom != v_to_sdom.at(w).vertex) {
                auto idom_it = v_to_idom_.find(idom);
                assert(idom_it != v_to_idom_.end());
                idom = idom_it->second;
            }
        }
    }

    VertexToIDomContainer v_to_idom_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_GRAPHS_DOMINATOR_TREE_HPP
