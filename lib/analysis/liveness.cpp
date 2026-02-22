#include <format>
#include <ostream>

#include "bjac/analysis/liveness.hpp"

namespace bjac {

std::ostream &operator<<(std::ostream &os, const Lifetime &lt) {
    return os << std::format("{}", lt);
}

} // namespace bjac
