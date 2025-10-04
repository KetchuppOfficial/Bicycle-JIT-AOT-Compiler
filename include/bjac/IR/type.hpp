#ifndef INCLUDE_BJAC_IR_TYPE_HPP
#define INCLUDE_BJAC_IR_TYPE_HPP

#include <format>
#include <string_view>
#include <utility>

namespace bjac {

enum class Type { kVoid, kI1, kI8, kI16, kI32, kI64 };

inline std::string_view to_string_view(Type type) {
    switch (type) {
    case Type::kVoid:
        return "void";
    case Type::kI1:
        return "i1";
    case Type::kI8:
        return "i8";
    case Type::kI16:
        return "i16";
    case Type::kI32:
        return "i32";
    case Type::kI64:
        return "i64";
    default:
        std::unreachable();
    }
}

} // namespace bjac

namespace std {

template <>
struct formatter<::bjac::Type> : public formatter<string_view> {
    template <typename ParseConstexpr>
    constexpr ParseConstexpr::iterator parse(ParseConstexpr &ctx) {
        return formatter<string_view>::parse(ctx);
    }

    template <class FmtContext>
    FmtContext::iterator format(::bjac::Type type, FmtContext &ctx) const {
        return formatter<string_view>::format(::bjac::to_string_view(type), ctx);
    }
};

} // namespace std

#endif // INCLUDE_BJAC_IR_TYPE_HPP
