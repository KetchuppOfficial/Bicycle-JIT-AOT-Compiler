#ifndef INCLUDE_BJAC_UTILS_ILIST_ITERATOR_HPP
#define INCLUDE_BJAC_UTILS_ILIST_ITERATOR_HPP

#include <cstddef>
#include <iterator>
#include <memory>

#include "bjac/utils/ilist_node.hpp"

namespace bjac {

template <typename ValueT>
class ilist_iterator final {
    using node_type = ilist_node<ValueT>;

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

static_assert(std::bidirectional_iterator<ilist_iterator<int>>);

} // namespace bjac

#endif // INCLUDE_BJAC_UTILS_ILIST_ITERATOR_HPP
