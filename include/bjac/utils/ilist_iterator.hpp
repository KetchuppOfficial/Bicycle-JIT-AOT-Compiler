#ifndef INCLUDE_BJAC_UTILS_ILIST_ITERATOR_HPP
#define INCLUDE_BJAC_UTILS_ILIST_ITERATOR_HPP

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>

#include "bjac/utils/ilist_node.hpp"

namespace bjac {

template <typename ValueT>
class ilist_iterator final {
  public:
    template <typename T>
    friend class ilist;

    template <typename T>
    friend class ilist_iterator;

    using difference_type = std::ptrdiff_t;
    using value_type = ValueT;
    using pointer = ValueT *;
    using reference = ValueT &;
    using iterator_category = std::bidirectional_iterator_tag;

    ilist_iterator() = default;

    ilist_iterator(const ilist_iterator<std::remove_const_t<value_type>> &other) noexcept
        requires std::is_const_v<value_type>
        : node_{other.node_} {}

    ilist_iterator(const ilist_iterator &) = default;
    ilist_iterator &operator=(const ilist_iterator &) = default;

    ilist_iterator(ilist_iterator &&) = default;
    ilist_iterator &operator=(ilist_iterator &&) = default;

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
    using node_type = ilist_node<std::remove_const_t<value_type>>;
    using node_pointer =
        std::conditional_t<std::is_const_v<value_type>, const node_type *, node_type *>;

    ilist_iterator(node_pointer node) noexcept : node_{node} {}

    node_pointer node_ = nullptr;
};

static_assert(std::bidirectional_iterator<ilist_iterator<int>>);

} // namespace bjac

#endif // INCLUDE_BJAC_UTILS_ILIST_ITERATOR_HPP
