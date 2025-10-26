#ifndef INCLUDE_BJAC_GRAPHS_LOOP_HPP
#define INCLUDE_BJAC_GRAPHS_LOOP_HPP

#include <concepts>
#include <memory>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace bjac {

template <typename VertexHandler>
class Loop final {
  public:
    explicit Loop(VertexHandler header) : header_(header) {}

    VertexHandler get_header() const { return header_; }
    void set_header(VertexHandler header) { header_ = header; }

    std::unsigned_integral auto vertices_count() const noexcept { return vertices_.size(); }
    std::ranges::forward_range auto vertices() const { return std::ranges::subrange(vertices_); }
    bool contains_vertex(VertexHandler v) const { return vertices_.contains(v); }
    void add_vertex(VertexHandler v) { vertices_.insert(v); }
    void remove_vertex(VertexHandler v) { vertices_.erase(v); }

    std::unsigned_integral auto inner_loops_count() const noexcept { return inner_loops_.size(); }
    std::ranges::forward_range auto inner_loops() const {
        return inner_loops_ | std::views::values |
               std::views::transform([](const auto &loop_ptr) static { return loop_ptr.get(); });
    }
    template <typename Self>
    auto &get_inner_loop(this Self &&self, VertexHandler header) {
        return std::forward_like<Self>(*self.inner_loops_.at(header));
    }
    void add_inner_loop(std::unique_ptr<Loop> loop) {
        inner_loops_.emplace(loop->get_header(), std::move(loop));
    }
    void remove_inner_loop(VertexHandler header) { inner_loops_.erase(header); }

  private:
    VertexHandler header_;
    std::unordered_set<VertexHandler> vertices_;
    std::unordered_map<VertexHandler, std::unique_ptr<Loop>> inner_loops_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_GRAPHS_LOOP_HPP
