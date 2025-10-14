#ifndef INCLUDE_BJAC_GRAPHS_DOMINATOR_TREE_HPP
#define INCLUDE_BJAC_GRAPHS_DOMINATOR_TREE_HPP

#include <algorithm>
#include <cassert>
#include <ranges>
#include <unordered_map>
#include <optional>

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

    DominatorTree(const G &g) {
        const dfs_type dfs{g, Traits::source(g)};
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

    const_iterator begin() const { return v_to_idom_.begin(); }
    const_iterator cbegin() const { return v_to_idom_.cbegin(); }
    const_iterator end() const { return v_to_idom_.end(); }
    const_iterator cend() const { return v_to_idom_.cend(); }

  private:
    using dfs_type = DFS<G, Traits>;
    using time_type = typename dfs_type::time_type;

    struct SDom {
        vertex_handler vertex;
        time_type time;
    };

    using VertexToSDomContainer = std::unordered_map<vertex_handler, SDom>;

    static VertexToSDomContainer compute_semidominators(const G &g, const dfs_type &dfs) {
        auto comp = [&dfs](vertex_handler v, vertex_handler u) {
            return dfs.info(v).get_discovery_time() < dfs.info(u).get_discovery_time();
        };

        VertexToSDomContainer v_to_sdom;
        v_to_sdom.reserve(dfs.size());

        std::vector<vertex_handler> visited_vertices;
        visited_vertices.reserve(dfs.size());

        for (vertex_handler w : std::views::reverse(dfs.search_order()) |
                                    std::views::take(dfs.size() - 1) | std::views::values) {
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

            auto sdom = [&] -> vertex_handler {
                if (sdom_candidates_1.empty()) {
                    assert(!sdom_candidates_2.empty());
                    return *std::ranges::min_element(sdom_candidates_2, comp);
                } else if (sdom_candidates_2.empty()) {
                    assert(!sdom_candidates_1.empty());
                    return *std::ranges::min_element(sdom_candidates_1, comp);
                } else {
                    vertex_handler min_1 = *std::ranges::min_element(sdom_candidates_1, comp);
                    vertex_handler min_2 = *std::ranges::min_element(sdom_candidates_2, comp);
                    return std::min(min_1, min_2, comp);
                }
            }();

            v_to_sdom.try_emplace(w, sdom, dfs.info(sdom).get_discovery_time());
            visited_vertices.push_back(w);
        }

        return v_to_sdom;
    }

    void compute_idoms(const dfs_type &dfs, const VertexToSDomContainer &v_to_sdom) {
        v_to_idom_.reserve(dfs.size());

        auto search_order = dfs.search_order();

        for (vertex_handler w : search_order | std::views::reverse |
                                    std::views::take(dfs.size() - 1) | std::views::values) {
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

        for (vertex_handler w : search_order | std::views::drop(1) | std::views::values) {
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
