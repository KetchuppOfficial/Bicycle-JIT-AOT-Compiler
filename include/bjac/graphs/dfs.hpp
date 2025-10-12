#ifndef INCLUDE_BJAC_GRAPHS_DFS_HPP
#define INCLUDE_BJAC_GRAPHS_DFS_HPP

#include <algorithm>
#include <cstddef>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

namespace bjac {

template <typename G, typename Traits>
class DFS final {
  public:
    using time_type = std::size_t;
    using vertex_handler = typename Traits::vertex_handler;

    class InfoNode final {
      public:
        InfoNode(vertex_handler v) : vertex_{v}, discovery_time_{0}, predecessor_{nullptr} {}

        vertex_handler get_vertex() const { return vertex_; }
        time_type get_discovery_time() const { return discovery_time_; }

      private:
        friend class DFS;
        friend class SpanningTreeAncestorIterator;

        vertex_handler vertex_;
        time_type discovery_time_;
        InfoNode *predecessor_;
    };

    class SpanningTreeAncestorIterator {
      public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = const DFS::InfoNode;
        using pointer = value_type *;
        using reference = value_type &;
        using difference_type = std::ptrdiff_t;

        // for compatibility with ranges algorithms
        SpanningTreeAncestorIterator() : v_info_{nullptr} {}

        SpanningTreeAncestorIterator(const DFS &dfs, vertex_handler v)
            : v_info_(std::addressof(dfs.info_.at(v))) {}

        reference operator*() const { return *v_info_; }
        pointer operator->() const { return v_info_; }

        SpanningTreeAncestorIterator &operator++() {
            v_info_ = v_info_->predecessor_;
            return *this;
        }

        SpanningTreeAncestorIterator operator++(int) {
            auto old = *this;
            ++(*this);
            return old;
        }

        bool operator==(const SpanningTreeAncestorIterator &rhs) const = default;

      private:
        const InfoNode *v_info_;
    };

  private:
    enum class Status : bool { not_visited, visited };
    using InfoContainer = std::unordered_map<vertex_handler, InfoNode>;

  public:
    using size_type = typename InfoContainer::size_type;
    using iterator = typename InfoContainer::iterator;
    using const_iterator = typename InfoContainer::const_iterator;

    // st stands for spanning tree
    using const_st_iterator = SpanningTreeAncestorIterator;
    using st_iterator = const_st_iterator;

    DFS(const G &g, vertex_handler source) : source_(source) {
        const auto n_vertices = Traits::n_vertices(g);

        info_.reserve(n_vertices);

        std::unordered_map<vertex_handler, Status> visit_table;
        visit_table.reserve(n_vertices);
        for (vertex_handler v : Traits::vertices(g)) {
            info_.try_emplace(v, v);
            visit_table.emplace(v, Status::not_visited);
        }

        std::vector<vertex_handler> stack;
        stack.reserve(n_vertices);

        stack.push_back(source);

        for (time_type time = 0; !stack.empty();) {
            const auto u = stack.back();
            auto &was_visited = visit_table.find(u)->second;
            if (was_visited == Status::visited) {
                stack.pop_back();
                continue;
            }

            was_visited = Status::visited;
            auto &u_info = info_.find(u)->second;
            u_info.discovery_time_ = ++time;
            search_order_.emplace(time, u);

            for (vertex_handler v : Traits::adjacent_vertices(g, u)) {
                if (visit_table.find(v)->second == Status::not_visited) {
                    info_.find(v)->second.predecessor_ = std::addressof(u_info);
                    stack.push_back(v);
                }
            }
        }
    }

    vertex_handler get_source() const { return source_; }

    const InfoNode &info(vertex_handler v) const { return info_.at(v); }
    vertex_handler at(time_type t) const { return search_order_.at(t); }

    size_type size() const noexcept { return info_.size(); }

    iterator begin() noexcept { return info_.begin(); }
    const_iterator begin() const noexcept { return info_.begin(); }
    const_iterator cbegin() const noexcept { return info_.cbegin(); }
    iterator end() noexcept { return info_.end(); }
    const_iterator end() const noexcept { return info_.end(); }
    const_iterator cend() const noexcept { return info_.cend(); }

    st_iterator st_begin(vertex_handler v) { return {*this, v}; }
    st_iterator st_begin(vertex_handler v) const { return {*this, v}; }
    st_iterator st_end() { return {*this, source_}; }
    st_iterator st_end() const { return {*this, source_}; }

    auto search_order() const { return std::ranges::subrange(search_order_); }

    bool is_ancestor_of(vertex_handler v, vertex_handler u) const {
        return std::any_of(st_begin(v), st_end(),
                           [u](auto &info_node) { return info_node.get_vertex() == u; });
    }

    bool is_proper_ancestor_of(vertex_handler v, vertex_handler u) const {
        return v != u && is_ancestor_of(v, u);
    }

  private:
    vertex_handler source_;
    InfoContainer info_;
    std::map<time_type, vertex_handler> search_order_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_GRAPHS_DFS_HPP
