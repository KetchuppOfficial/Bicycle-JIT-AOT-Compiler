#ifndef INCLUDE_BJAC_UTILS_ILIST_HPP
#define INCLUDE_BJAC_UTILS_ILIST_HPP

#include <iterator>

#include "bjac/utils/ilist_iterator.hpp"
#include "bjac/utils/ilist_node.hpp"

namespace bjac {

template <typename ValueT>
class ilist {
  public:
    using node_type = ilist_node<ValueT>;
    using typename node_type::parent_type;
    using typename node_type::value_type;
    using iterator = ilist_iterator<ValueT>;
    using const_iterator = ilist_iterator<const ValueT>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    ilist() noexcept { sentinel_.set_next(&sentinel_); }

    iterator begin() noexcept { return {head()}; }
    const_iterator begin() const noexcept { return {head()}; }
    const_iterator cbegin() const noexcept { return begin(); }

    reverse_iterator rbegin() noexcept { return {end()}; }
    const_reverse_iterator rbegin() const noexcept { return {end()}; }
    const_reverse_iterator crbegin() const noexcept { return rbegin(); }

    iterator end() noexcept { return {&sentinel_}; }
    const_iterator end() const noexcept { return {&sentinel_}; }
    const_iterator cend() const noexcept { return end(); }

    reverse_iterator rend() noexcept { return {begin()}; }
    const_reverse_iterator rend() const noexcept { return {begin()}; }
    const_reverse_iterator crend() const noexcept { return rend(); }

  private:
    auto *head() noexcept { return sentinel_.next(); }
    auto *head() const noexcept { return sentinel_.next(); }

    node_type sentinel_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_UTILS_ILIST_HPP
