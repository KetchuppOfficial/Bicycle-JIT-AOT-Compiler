#include <ostream>
#include <ranges>

#include "bjac/IR/function.hpp"

namespace bjac {

void Function::print(std::ostream &os) const {
    auto args =
        arguments_ | std::views::transform([](Type type) static { return to_string_view(type); });
    std::println(os, "{} {}({:s})", return_type_, name_, std::views::join_with(args, ", "));
    for (auto &bb : *this) {
        bb.print(os);
    }
}

std::ostream &operator<<(std::ostream &os, const Function &f) {
    f.print(os);
    return os;
}

} // namespace bjac
