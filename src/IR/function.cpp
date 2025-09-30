#include <print>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>

#include "fmt/ostream.h"
#include "fmt/ranges.h"

#include "bjac/IR/function.hpp"
#include "bjac/IR/type.hpp"

namespace fmt {

template <>
struct formatter<bjac::Type> : public formatter<std::string_view> {
    template <typename ParseConstexpr>
    constexpr ParseConstexpr::iterator parse(ParseConstexpr &ctx) {
        return formatter<std::string_view>::parse(ctx);
    }

    template <class FmtContext>
    FmtContext::iterator format(bjac::Type type, FmtContext &ctx) const {
        return formatter<std::string_view>::format(bjac::to_string_view(type), ctx);
    }
};

} // namespace fmt

namespace bjac {

void Function::print(std::ostream &os) const {
    fmt::println(os, "{} {}({}):", return_type_, name_, fmt::join(arguments_, ", "));
    for (auto &bb : *this) {
        auto preds_str = [&] {
            auto preds = bb.predecessors();
            if (preds.empty()) {
                return std::string{};
            }

            std::ostringstream oss;
            for (auto *pred : std::views::take(preds, preds.size() - 1)) {
                std::print(oss, "%bb{}, ", pred->get_id().value());
            }
            std::print(oss, "%bb{}", preds.back()->get_id().value());

            return oss.str();
        }();

        std::println(os, "%bb{}: preds: {}", bb.get_id().value(), preds_str);
        for (auto &instr : bb) {
            std::println(os, "    {}", instr.to_string());
        }
    }
}

} // namespace bjac
