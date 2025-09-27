#ifndef INCLUDE_BJAC_UTILS_ILIST_HPP
#define INCLUDE_BJAC_UTILS_ILIST_HPP

#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

#include "bjac/utils/ilist_iterator.hpp"
#include "bjac/utils/ilist_node.hpp"

namespace bjac {

template <typename ValueT>
class ilist {
    using node_type = ilist_node<ValueT>;

  public:
    using value_type = ValueT;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = ilist_iterator<ValueT>;
    using const_iterator = ilist_iterator<const ValueT>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = std::size_t;
    using difference_type = iterator::difference_type;

    ilist() = default;

    ilist(const ilist &) = delete;
    ilist &operator=(const ilist &) = delete;

    ilist(ilist &&rhs) noexcept
        : sentinel_{rhs.empty() ? this : rhs.head(), rhs.empty() ? this : rhs.tail()},
          size_{std::exchange(rhs.size_, 0)} {}

    ilist &operator=(ilist &&rhs) noexcept {
        swap(rhs);
        return *this;
    }

    ~ilist() {
        auto *current = head();
        auto *next = current->next();
        for (; current != &sentinel_; current = std::exchange(next, next->next())) {
            delete current;
        }
    }

    bool empty() const noexcept { return head() == tail(); }
    size_type size() const noexcept { return size_; }

    void swap(ilist &other) noexcept {
        if (empty()) {
            if (!other.empty()) {
                swap_sentinels_one_of_which_is_empty(other);
            }
        } else if (other.empty()) {
            other.swap_sentinels_one_of_which_is_empty(*this);
        } else {
            sentinel_.swap(other.sentinel_);
        }

        std::swap(size_, other.size_);
    }

    template <typename Self>
    auto &front(this Self &&self) {
        assert(!self.empty());
        return self.head()->value();
    }

    template <typename Self>
    auto &back(this Self &&self) {
        assert(!self.empty());
        return self.tail()->value();
    }

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

    template <typename... Args>
    iterator emplace(const_iterator pos, Args &&...args) {
        node_type *new_node = new value_type(std::forward<Args>(args)...);

        auto *next = const_cast<node_type *>(pos.node_);
        auto *prev = next->prev();

        prev->set_next(new_node);
        next->set_prev(new_node);
        new_node->set_next(next);
        new_node->set_prev(prev);

        return iterator{new_node};
    }

    iterator insert(const_iterator pos, const value_type &value) { return emplace(pos, value); }
    iterator insert(const_iterator pos, value_type &&value) {
        return emplace(pos, std::move(value));
    }

    template <typename... Args>
    reference emplace_back(Args &&...args) {
        return *emplace(end(), std::forward<Args>(args)...);
    }

    void push_back(const value_type &value) { emplace(end(), value); }
    void push_back(value_type &&value) { emplace(end(), std::move(value)); }

    template <typename... Args>
    reference emplace_front(Args &&...args) {
        return *emplace(begin(), std::forward<Args>(args)...);
    }

    void push_front(const value_type &value) { emplace(begin(), value); }
    void push_front(value_type &&value) { emplace(begin(), std::move(value)); }

    iterator erase(const_iterator pos) {
        assert(pos != end());

        auto *to_erase = const_cast<node_type *>(pos.node_);
        auto *prev = to_erase->prev();
        auto *next = to_erase->next();

        prev->set_next(next);
        next->set_prev(prev);
        delete to_erase;

        return {next};
    }

    iterator erase(iterator pos) {
        static_assert(!std::is_same_v<iterator, const_iterator>);
        return erase(const_iterator{pos});
    }

    void pop_front() { erase(begin()); }
    void pop_back() { erase(const_iterator{tail()}); }

  private:
    void swap_sentinels_one_of_which_is_empty(ilist &other) {
        assert(empty());
        assert(!other.empty());

        sentinel_.set_next(other.sentinel_->next());
        sentinel_.set_prev(other.sentinel_->prev());
        other.sentinel_.reset();
    }

    template <typename Self>
    auto *head(this Self &&self) noexcept {
        return self.sentinel_.next();
    }

    template <typename Self>
    auto *tail(this Self &&self) noexcept {
        return self.sentinel_.prev();
    }

    node_type sentinel_;
    size_type size_ = 0;
};

} // namespace bjac

#endif // INCLUDE_BJAC_UTILS_ILIST_HPP
