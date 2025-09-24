#ifndef INCLUDE_BJAC_UTILS_ILIST_NODE_HPP
#define INCLUDE_BJAC_UTILS_ILIST_NODE_HPP

#include <type_traits>
#include <utility>

namespace bjac {

template <typename ValueT>
class ilist_node {
  public:
    using value_type = ValueT;

    static_assert(std::is_object_v<value_type>);

    ilist_node() = default;

    ilist_node(const ilist_node &) = delete;
    ilist_node &operator=(const ilist_node &) = delete;

    ilist_node(ilist_node &&rhs) noexcept
        : next_{std::exchange(rhs.next_, nullptr)}, prev_{std::exchange(rhs.prev_, nullptr)} {}

    ilist_node &operator=(ilist_node &&rhs) noexcept {
        if (this != std::addressof(rhs)) {
            next_ = std::exchange(rhs.next_, nullptr);
            prev_ = std::exchange(rhs.prev_, nullptr);
        }
        return *this;
    }

    ilist_node *next() noexcept { return next_; }
    const ilist_node *next() const noexcept { return next_; }
    void set_next(ilist_node *next) noexcept { next_ = next; }

    ilist_node *prev() noexcept { return prev_; }
    const ilist_node *prev() const noexcept { return prev_; }
    void set_prev(ilist_node *prev) noexcept { prev_ = prev; }

    // Note: this methods should be called only if ilist_node is a base class subobject of ValueT
    auto &value() noexcept { return *static_cast<value_type *>(this); }
    const auto &value() const noexcept { return *static_cast<const value_type *>(this); }

  private:
    ilist_node *next_ = nullptr;
    ilist_node *prev_ = nullptr;
};

} // namespace bjac

#endif // INCLUDE_BJAC_UTILS_ILIST_NODE_HPP
