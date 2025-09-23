#ifndef INCLUDE_BJAC_UTILS_ILIST_HPP
#define INCLUDE_BJAC_UTILS_ILIST_HPP

#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace bjac {

template <typename ValueT, typename ParentT>
class ilist_node {
  public:
    using parent_type = std::remove_cv_t<ParentT>;
    using value_type = ValueT;

    static_assert(std::is_object_v<parent_type>);
    static_assert(std::is_object_v<value_type>);
    static_assert(std::is_base_of_v<ilist_node, value_type>);

    ilist_node() = default;

    ilist_node(const ilist_node &) = delete;
    ilist_node &operator=(const ilist_node &) = delete;

    ilist_node(ilist_node &&rhs) noexcept
        : next_{std::exchange(rhs.next_, nullptr)}, prev_{std::exchange(rhs.prev_, nullptr)},
          parent_{std::exchange(rhs.parent_, nullptr)} {}

    ilist_node &operator=(ilist_node &&rhs) noexcept {
        if (this != std::addressof(rhs)) {
            next_ = std::exchange(rhs.next_, nullptr);
            prev_ = std::exchange(rhs.prev_, nullptr);
            parent_ = std::exchange(rhs.parent_, nullptr);
        }
        return *this;
    }

    ilist_node *next() noexcept { return next_; }
    const ilist_node *next() const noexcept { return next_; }
    void set_next(ilist_node *next) noexcept { next_ = next; }

    ilist_node *prev() noexcept { return prev_; }
    const ilist_node *prev() const noexcept { return prev_; }
    void set_prev(ilist_node *prev) noexcept { prev_ = prev; }

    parent_type *parent() noexcept { return parent_; }
    const parent_type *parent() const noexcept { return parent_; }
    void set_parent(parent_type *parent) noexcept { parent_ = parent; }

    // Note: this methods should be called only if ilist_node is a base class subobject of ValueT
    auto &value() noexcept { return *static_cast<value_type *>(this); }
    const auto &value() const noexcept { return *static_cast<const value_type *>(this); }

  private:
    ilist_node *next_ = nullptr;
    ilist_node *prev_ = nullptr;
    ParentT *parent_ = nullptr;
};

template <typename ValueT, typename ParentT>
class ilist_iterator final {
    using node_type = ilist_node<ValueT, ParentT>;

  public:
    using difference_type = std::ptrdiff_t;
    using value_type = ValueT;
    using pointer = ValueT *;
    using reference = ValueT &;
    using iterator_category = std::bidirectional_iterator_tag;

    ilist_iterator() = default;
    ilist_iterator(node_type *node) noexcept : node_{node} {}

    reference operator*() const noexcept { return node_->value(); }
    pointer operator->() const noexcept { return std::addressof(node_->value()); }

    ilist_iterator &operator++() { return (node_ = node_->next(), *this); }
    ilist_iterator operator++(int) {
        auto old = *this;
        ++*this;
        return old;
    }

    ilist_iterator &operator--() { return (node_ = node_->prev(), *this); }
    ilist_iterator operator--(int) {
        auto old = *this;
        --*this;
        return old;
    }

    friend bool operator==(ilist_iterator lhs, ilist_iterator rhs) = default;

  private:
    node_type *node_ = nullptr;
};

static_assert(std::bidirectional_iterator<ilist_iterator<int, int>>);

template <typename ValueT, typename ParentT>
class ilist : public ilist_node<ValueT, ParentT> {
  public:
    using node_type = ilist_node<ValueT, ParentT>;
    using typename node_type::parent_type;
    using typename node_type::value_type;
    using iterator = ilist_iterator<ValueT, ParentT>;
    using const_iterator = ilist_iterator<const ValueT, ParentT>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    ilist() noexcept { sentinel()->set_next(sentinel()); }

    iterator begin() noexcept { return {head()}; }
    const_iterator begin() const noexcept { return {head()}; }
    const_iterator cbegin() const noexcept { return begin(); }

    reverse_iterator rbegin() noexcept { return {end()}; }
    const_reverse_iterator rbegin() const noexcept { return {end()}; }
    const_reverse_iterator crbegin() const noexcept { return rbegin(); }

    iterator end() noexcept { return {sentinel()}; }
    const_iterator end() const noexcept { return {sentinel()}; }
    const_iterator cend() const noexcept { return end(); }

    reverse_iterator rend() noexcept { return {begin()}; }
    const_reverse_iterator rend() const noexcept { return {begin()}; }
    const_reverse_iterator crend() const noexcept { return rend(); }

  private:
    auto *head() noexcept { return sentinel()->next(); }
    auto *head() const noexcept { return sentinel()->next(); }

    auto *sentinel() noexcept { return static_cast<node_type *>(this); }
    auto *sentinel() const noexcept { return static_cast<const node_type *>(this); }
};

} // namespace bjac

#endif // INCLUDE_BJAC_UTILS_ILIST_HPP
