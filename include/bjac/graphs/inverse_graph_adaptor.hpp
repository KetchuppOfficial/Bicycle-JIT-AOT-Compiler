#ifndef INCLUDE_BJAC_GRAPHS_INVERSE_GRAPH_ADAPTOR_HPP
#define INCLUDE_BJAC_GRAPHS_INVERSE_GRAPH_ADAPTOR_HPP

#include <ranges>
#include <unordered_map>
#include <vector>

namespace bjac {

template <typename G, typename Traits>
class InverseGraphAdaptor final {
  public:
    using vertex_handler = typename Traits::vertex_handler;

  private:
    using edges_container = std::unordered_map<vertex_handler, std::vector<vertex_handler>>;

  public:
    using size_type = typename edges_container::size_type;

    explicit InverseGraphAdaptor(const G &g) {
        edges_.reserve(Traits::n_vertices(g));
        for (vertex_handler v : Traits::vertices(g)) {
            auto &adjacent_vertices = edges_.try_emplace(v).first->second;
            for (vertex_handler u : Traits::predecessors(g, v)) {
                adjacent_vertices.push_back(u);
            }
        }
    }

    size_type n_vertices() const noexcept { return edges_.size(); }

    std::ranges::forward_range auto vertices() const { return std::views::keys(edges_); };
    std::ranges::forward_range auto adjacent_vertices(vertex_handler v) const {
        return std::ranges::subrange(edges_.at(v));
    }

  private:
    edges_container edges_;
};

template <typename G, typename Traits>
struct InverseGraphAdaptorTraits {
    using graph_type = InverseGraphAdaptor<G, Traits>;
    using size_type = typename graph_type::size_type;
    using vertex_handler = typename graph_type::vertex_handler;

    static size_type n_vertices(const graph_type &g) { return g.n_vertices(); }
    static std::ranges::forward_range auto vertices(const graph_type &g) { return g.vertices(); }
    static std::ranges::forward_range auto adjacent_vertices(const graph_type &g,
                                                             vertex_handler v) {
        return g.adjacent_vertices(v);
    }
};

} // namespace bjac

#endif // INCLUDE_BJAC_GRAPHS_INVERSE_GRAPH_ADAPTOR_HPP
