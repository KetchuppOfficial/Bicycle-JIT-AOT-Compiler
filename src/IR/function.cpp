#include <print>
#include <ranges>
#include <sstream>
#include <string>

#include "bjac/IR/function.hpp"

namespace bjac {

void Function::print(std::ostream &os) const {
    std::println(os, "{} {}({:n})", return_type_, name_, arguments_);
    for (auto &bb : *this) {
        const auto preds_str = [&] {
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
