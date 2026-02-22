#ifndef INCLUDE_BJAC_ANALYSIS_LIVENESS_HPP
#define INCLUDE_BJAC_ANALYSIS_LIVENESS_HPP

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

    Lifetime() = default;

    Lifetime(std::initializer_list<Segment> ilist) {
        for (const auto &seg : ilist) {
            add(seg);
        }
    }

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

    std::ranges::forward_range auto view() const { return std::views::all(lt_segments_); }

    void add(Segment seg) {
        auto [first, last] = lt_segments_.equal_range(seg);
        while (first != last) {
            seg = Segment{std::min(seg.start(), first->start()), std::max(seg.end(), first->end())};
            lt_segments_.erase(std::exchange(first, std::next(first)));
        }
        lt_segments_.insert(seg);
    }

    friend std::ostream &operator<<(std::ostream &os, const Lifetime &lt);

  private:
    struct Compare {
        bool operator()(const Segment &lhs, const Segment &rhs) const noexcept {
            return lhs.end() < rhs.start();
        }
    };

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
        auto strings = lt.view() | std::views::transform([](const auto &seg) static {
                           return std::format("{}", seg);
                       });
        return format_to(ctx.out(), "{:s}", std::views::join_with(strings, " U "sv));
    }
};

} // namespace std

#endif // INCLUDE_BJAC_ANALYSIS_LIVENESS_HPP
