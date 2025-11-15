#ifndef INCLUDE_BJAC_GRAPHS_GRAPH_TRAITS_HPP
#define INCLUDE_BJAC_GRAPHS_GRAPH_TRAITS_HPP

#include <ranges>

namespace bjac {

// clang-format off

// This class is useless on its own but serves as the documentation for what is expected from a
// graph traits class.
struct DefaultGraphTraits {
/*
 * The following nested types shall be defined:
 *
 * // the type of the graph
 * using graph_type = ...;
 *
 * // an unsigned integer type
 * using size_type = ...;
 *
 * // some type to access a vertex; it's expected to be copyable and hashable
 * using vertex_handler = ...;
 *
 *
 * The following static member functions shall be defined:
 *
 * // returns the number of vertices in a graph
 * static size_type n_vertices(const graph_type &g);
 *
 * // returns a range which value type is vertex_handler and which contains all vertices in the
 * // given graph
 * static std::ranges::forward_range auto vertices(graph_type &g);
 *
 * // returns a range which value type is vertex_handler and which contains all adjacent vertices
 * // of the given one in the given graph
 * static std::ranges::forward_range auto adjacent_vertices(graph_type &g, vertex_handler v);
 *
 * // returns a range which value type is vertex_handler and which contains all such vertices u that
 * // edges (u, v) are edges of the graph g
 * static std::ranges::forward_range auto predecessors(graph_type &g, vertex_handler v);
 *
 * // returns the source vertex of a flow graph (provided only for flow graphs)
 * static vertex_handler source(const G &g);
 */
};

// clang-format on

template <typename Traits>
struct ReverseGraphTraits {
    using graph_type = typename Traits::graph_type;
    using size_type = typename Traits::size_type;
    using vertex_handler = typename Traits::vertex_handler;

    static size_type n_vertices(const graph_type &g) { return g.size(); }
    static std::ranges::forward_range auto vertices(graph_type &g) { return Traits::vertices(g); }

    static std::ranges::forward_range auto adjacent_vertices([[maybe_unused]] graph_type &g,
                                                             vertex_handler v) {
        return Traits::predecessors(g, v);
    }

    static std::ranges::forward_range auto predecessors([[maybe_unused]] graph_type &g,
                                                        vertex_handler v) {
        return Traits::adjacent_vertices(g, v);
    }

    static vertex_handler source(graph_type &g) { return Traits::source(g); }
};

} // namespace bjac

#endif // INCLUDE_BJAC_GRAPHS_GRAPH_TRAITS_HPP
