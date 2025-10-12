#ifndef INCLUDE_BJAC_UTILS_ILIST_HPP
#define INCLUDE_BJAC_UTILS_ILIST_HPP

#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
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

    ilist() noexcept : sentinel_{&sentinel_, &sentinel_}, size_{0} {}

    ilist(const ilist &) = delete;
    ilist &operator=(const ilist &) = delete;

    ilist(ilist &&rhs) noexcept
        : sentinel_{rhs.empty() ? &sentinel_ : rhs.head(), rhs.empty() ? &sentinel_ : rhs.tail()},
          size_{std::exchange(rhs.size_, 0)} {
        fixup_head_and_tail();
    }

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
            if (other.empty()) {
                return;
            }
            swap_sentinels_one_of_which_is_empty(other);
            fixup_head_and_tail();
            size_ = std::exchange(other.size_, 0);
        } else if (other.empty()) {
            other.swap_sentinels_one_of_which_is_empty(*this);
            other.fixup_head_and_tail();
            other.size_ = std::exchange(size_, 0);
        } else {
            sentinel_.swap(other.sentinel_);
            fixup_head_and_tail();
            other.fixup_head_and_tail();
            std::swap(size_, other.size_);
        }
    }

    // Note: C-style cast is necessary because other classes may inherit from ilist privately
    template <typename Self>
    auto &front(this Self &&self) {
        assert(!self.empty());
        using Base = decltype(std::forward_like<Self>(std::declval<ilist>()));
        return ((Base)self).head()->value();
    }

    // Note: C-style cast is necessary because other classes may inherit from ilist privately
    template <typename Self>
    auto &back(this Self &&self) {
        assert(!self.empty());
        using Base = decltype(std::forward_like<Self>(std::declval<ilist>()));
        return ((Base)self).tail()->value();
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

    template <typename T, typename... Args>
    iterator emplace(const_iterator pos, Args &&...args) {
        static_assert(std::is_base_of_v<value_type, T>);
        node_type *new_node = new T(std::forward<Args>(args)...);
        return insert_node(pos, new_node);
    }

    template <typename T, typename... Args>
    reference emplace_back(Args &&...args) {
        return *emplace<T>(end(), std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    reference emplace_front(Args &&...args) {
        return *emplace<T>(begin(), std::forward<Args>(args)...);
    }

    iterator insert(const_iterator pos, std::unique_ptr<node_type> node) {
        assert(node);
        return insert_node(pos, node.release());
    }

    reference push_back(std::unique_ptr<node_type> node) { return *insert(end(), std::move(node)); }
    reference push_front(std::unique_ptr<node_type> node) {
        return *insert(begin(), std::move(node));
    }

    iterator erase(const_iterator pos) {
        assert(pos != end());

        auto *to_erase = const_cast<node_type *>(pos.node_);
        auto *prev = to_erase->prev();
        auto *next = to_erase->next();

        prev->set_next(next);
        next->set_prev(prev);
        delete to_erase;

        --size_;

        return {next};
    }

    iterator erase(iterator pos) {
        static_assert(!std::is_same_v<iterator, const_iterator>);
        return erase(const_iterator{pos});
    }

    void pop_front() { erase(begin()); }
    void pop_back() { erase(const_iterator{tail()}); }

  private:
    iterator insert_node(const_iterator pos, node_type *new_node) noexcept {
        auto *next = const_cast<node_type *>(pos.node_);
        auto *prev = next->prev();

        prev->set_next(new_node);
        next->set_prev(new_node);
        new_node->set_next(next);
        new_node->set_prev(prev);

        ++size_;

        return iterator{new_node};
    }

    void swap_sentinels_one_of_which_is_empty(ilist &other) noexcept {
        assert(empty());
        assert(!other.empty());

        sentinel_.set_next(other.sentinel_->next());
        sentinel_.set_prev(other.sentinel_->prev());
        other.sentinel_.reset();
    }

    void fixup_head_and_tail() noexcept {
        head()->set_prev(&sentinel_);
        tail()->set_next(&sentinel_);
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
