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

    ilist_node() noexcept : ilist_node{nullptr, nullptr} {}
    ilist_node(ilist_node *next, ilist_node *prev) noexcept : next_{next}, prev_{prev} {}

    ilist_node(const ilist_node &) = delete;
    ilist_node &operator=(const ilist_node &) = delete;

    ilist_node(ilist_node &&rhs) = delete;
    ilist_node &operator=(ilist_node &&rhs) = delete;

    virtual ~ilist_node() = default;

    void reset() noexcept { next_ = prev_ = this; }
    void swap(ilist_node &other) noexcept {
        std::swap(next_, other.next_);
        std::swap(prev_, other.prev_);
    }

    template <typename Self>
    auto *next(this Self &&self) noexcept {
        return std::addressof(std::forward_like<Self>(*self.next_));
    }

    void set_next(ilist_node *next) noexcept { next_ = next; }

    template <typename Self>
    auto *prev(this Self &&self) noexcept {
        return std::addressof(std::forward_like<Self>(*self.prev_));
    }

    void set_prev(ilist_node *prev) noexcept { prev_ = prev; }

    // Note: this methods should be called only if ilist_node is a base class subobject of ValueT
    template <typename Self>
    auto &value(this Self &&self) noexcept {
        using derived = std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>,
                                           const value_type, value_type>;
        return static_cast<derived &>(self);
    }

  private:
    ilist_node *next_;
    ilist_node *prev_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_UTILS_ILIST_NODE_HPP
