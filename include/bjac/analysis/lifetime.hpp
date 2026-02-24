#ifndef INCLUDE_BJAC_ANALYSIS_LIFETIME_HPP
#define INCLUDE_BJAC_ANALYSIS_LIFETIME_HPP

#include <cstddef>
#include <format>
#include <initializer_list>
#include <iosfwd>
#include <iterator>
#include <ranges>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace bjac {

class Lifetime final {
  public:
    class Segment final {
      public:
        explicit Segment(std::size_t start, std::size_t end) : start_{start}, end_{end} {
            if (start > end) {
                throw std::invalid_argument{
                    std::format("[{}; {}] is not a valid segment", start, end)};
            }
        }

        std::size_t start() const noexcept { return start_; }
        std::size_t end() const noexcept { return end_; }

        bool operator==(const Segment &) const = default;

      private:
        std::size_t start_;
        std::size_t end_;
    };

  private:
    struct Compare {
        // This comparator enables treating intersecting and adjacent segments as equivalent, hence
        // easily merging them
        bool operator()(const Segment &lhs, const Segment &rhs) const noexcept {
            return lhs.end() + 1 < rhs.start();
        }
    };

  public:
    using const_iterator = std::set<Segment, Compare>::const_iterator;
    using iterator = const_iterator;
    using const_reverse_iterator = std::set<Segment, Compare>::const_reverse_iterator;
    using reverse_iterator = const_reverse_iterator;

    Lifetime() = default;

    Lifetime(std::initializer_list<Segment> ilist) {
        for (const auto &seg : ilist) {
            add(seg);
        }
    }

    const_iterator begin() const noexcept { return lt_segments_.begin(); }
    const_iterator cbegin() const noexcept { return begin(); }
    const_reverse_iterator rbegin() const noexcept { return lt_segments_.rbegin(); }
    const_reverse_iterator crbegin() const noexcept { return rbegin(); }

    const_iterator end() const noexcept { return lt_segments_.end(); }
    const_iterator cend() const noexcept { return end(); }
    const_reverse_iterator rend() const noexcept { return lt_segments_.rend(); }
    const_reverse_iterator crend() const noexcept { return rend(); }

    // Checks whether given point/segment belongs to the lifetime set
    bool intersects(std::size_t point) const { return intersects(Segment{point, point}); }
    bool intersects(const Segment &seg) const { return lt_segments_.contains(seg); }

    // Checks whether given segment [a; b] belongs to the lifetime set and no segment [a - e; b + e]
    // belongs to the lifetime set for any positive value e (a == b for the case of a point)
    bool contains(std::size_t point) const { return contains(Segment{point, point}); }
    bool contains(const Segment &seg) const {
        auto it = lt_segments_.find(seg);
        return it != lt_segments_.end() && *it == seg;
    }

    auto find(const Segment &seg) const { return lt_segments_.find(seg); }

    void add(Segment seg) {
        auto [first, last] = lt_segments_.equal_range(seg);
        while (first != last) {
            seg = Segment{std::min(seg.start(), first->start()), std::max(seg.end(), first->end())};
            lt_segments_.erase(std::exchange(first, std::next(first)));
        }
        lt_segments_.insert(seg);
    }

    void remove(Segment seg) {
        if (auto it = lt_segments_.find(seg); it != lt_segments_.end() && *it == seg) {
            lt_segments_.erase(it);
        }
    }

    void remove(const_iterator it) { lt_segments_.erase(it); }

    bool operator==(const Lifetime &) const = default;

    friend std::ostream &operator<<(std::ostream &os, const Lifetime &lt);

  private:
    std::set<Segment, Compare> lt_segments_;
};

} // namespace bjac

namespace std {

template <>
struct formatter<::bjac::Lifetime::Segment> : formatter<string> {
    using formatter<string>::parse;

    template <typename FmContext>
    typename FmContext::iterator format(const ::bjac::Lifetime::Segment &seg,
                                        FmContext &ctx) const {
        return format_to(ctx.out(), "[{}; {}]", seg.start(), seg.end());
    }
};

template <>
struct formatter<::bjac::Lifetime> : formatter<string> {
    using formatter<string>::parse;

    template <typename FmContext>
    typename FmContext::iterator format(const ::bjac::Lifetime &lt, FmContext &ctx) const {
        using namespace std::string_view_literals;
        auto strings = lt | std::views::transform(
                                [](const auto &seg) static { return std::format("{}", seg); });
        return format_to(ctx.out(), "{:s}", std::views::join_with(strings, " U "sv));
    }
};

} // namespace std

namespace bjac {

inline std::ostream &operator<<(std::ostream &os, const Lifetime &lt) {
    return os << std::format("{}", lt);
}

} // namespace bjac

#endif // INCLUDE_BJAC_ANALYSIS_LIFETIME_HPP
