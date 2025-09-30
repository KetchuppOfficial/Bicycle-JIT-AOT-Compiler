#include <ostream>
#include <print>
#include <ranges>
#include <string>

#include "bjac/IR/function.hpp"

namespace bjac {

void Function::print(std::ostream &os) const {
    std::println(os, "{} {}({:n})", return_type_, name_, arguments_);
    for (auto &bb : *this) {
        const auto preds_str = [preds = bb.predecessors()] {
            if (preds.empty()) {
                return std::string{};
            }

            std::string preds_str;

            constexpr std::size_t kCharsForOneBB = 8; // strlen("%bb, ") + 3 chars for ID
            preds_str.reserve(kCharsForOneBB * preds.size());

            for (const auto *pred : std::views::take(preds, preds.size() - 1)) {
                preds_str += std::format("%bb{}, ", pred->get_id().value());
            }
            preds_str += std::format("%bb{}", preds.back()->get_id().value());

            return preds_str;
        }();

        std::println(os, "%bb{}: preds: {}", bb.get_id().value(), preds_str);
        for (auto &instr : bb) {
            std::println(os, "    {}", instr.to_string());
        }
    }
}

} // namespace bjac
