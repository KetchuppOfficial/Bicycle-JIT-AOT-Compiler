#include <ostream>

#include "bjac/IR/function.hpp"

namespace bjac {

void Function::print(std::ostream &os) const {
    std::println(os, "{} {}({:n})", return_type_, name_, arguments_);
    for (auto &bb : *this) {
        bb.print(os);
    }
}

std::ostream &operator<<(std::ostream &os, const Function &f) {
    f.print(os);
    return os;
}

} // namespace bjac
