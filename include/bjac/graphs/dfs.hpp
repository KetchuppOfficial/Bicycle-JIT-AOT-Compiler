#ifndef INCLUDE_BJAC_GRAPHS_DFS_HPP
#define INCLUDE_BJAC_GRAPHS_DFS_HPP

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace bjac {

template <typename Traits>
class DFS final {
  public:
    using time_type = std::size_t;
    using graph_type = typename Traits::graph_type;
    using vertex_handler = typename Traits::vertex_handler;

    class InfoNode final {
      public:
        InfoNode(vertex_handler v) : InfoNode{v, nullptr} {}
        InfoNode(vertex_handler v, InfoNode *predecessor)
            : vertex_(v), discovery_time_{0}, finished_time_{0}, predecessor_{predecessor} {}

        vertex_handler get_vertex() const { return vertex_; }
        time_type get_discovery_time() const { return discovery_time_; }
        time_type get_finished_time() const { return finished_time_; }

      private:
        friend class DFS;
        friend class SpanningTreeAncestorIterator;

        vertex_handler vertex_;
        time_type discovery_time_;
        time_type finished_time_;
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
    using InfoContainer = std::unordered_map<vertex_handler, InfoNode>;

  public:
    using size_type = typename InfoContainer::size_type;
    using iterator = typename InfoContainer::iterator;
    using const_iterator = typename InfoContainer::const_iterator;

    // st stands for spanning tree
    using const_st_iterator = SpanningTreeAncestorIterator;
    using st_iterator = const_st_iterator;

    // Note: vertices from already_visited appear only in post_order, not in pre_order
    DFS(graph_type &g, vertex_handler source,
        std::initializer_list<vertex_handler> already_visited = {})
        : source_(source) {
        std::unordered_set<vertex_handler> visited_vertices{already_visited};

        std::vector<vertex_handler> stack;
        stack.reserve(Traits::n_vertices(g));

        info_.emplace(source_, source_);
        stack.push_back(source_);

        for (time_type time = 0; !stack.empty();) {
            vertex_handler u = stack.back();
            auto &u_info = info_.find(u)->second;

            if (visited_vertices.insert(u).second) {
                u_info.discovery_time_ = ++time;
                pre_order_.push_back(u);

                for (vertex_handler v : Traits::adjacent_vertices(g, u)) {
                    if (!visited_vertices.contains(v)) {
                        info_.try_emplace(v, v, std::addressof(u_info));
                        stack.push_back(v);
                    }
                }
            } else {
                stack.pop_back();
                u_info.finished_time_ = ++time;
                post_order_.push_back(u);
            }
        }
    }

    DFS(graph_type &g, std::initializer_list<vertex_handler> already_visited = {})
        : DFS(g, Traits::source(g), already_visited) {}

    vertex_handler get_source() const { return source_; }

    bool contains(vertex_handler v) const { return info_.contains(v); }

    const InfoNode &info(vertex_handler v) const { return info_.at(v); }

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

    auto pre_order() const { return std::ranges::subrange(pre_order_); }
    auto post_order() const { return std::ranges::subrange(post_order_); }

    bool is_ancestor_of(vertex_handler v, vertex_handler u) const {
        const auto &v_info = info_.at(v);
        const auto &u_info = info_.at(u);

        return u_info.get_discovery_time() <= v_info.get_discovery_time() &&
               v_info.get_finished_time() <= u_info.get_finished_time();
    }

    bool is_proper_ancestor_of(vertex_handler v, vertex_handler u) const {
        return v != u && is_ancestor_of(v, u);
    }

  private:
    vertex_handler source_;
    InfoContainer info_;
    std::vector<vertex_handler> pre_order_;
    std::vector<vertex_handler> post_order_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_GRAPHS_DFS_HPP
