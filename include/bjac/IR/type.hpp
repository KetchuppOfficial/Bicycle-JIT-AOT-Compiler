#ifndef INCLUDE_BJAC_IR_TYPE_HPP
#define INCLUDE_BJAC_IR_TYPE_HPP

#include <format>
#include <memory>
#include <string_view>
#include <utility>

namespace bjac {

class Type {
  public:
    enum class ID {
        kNone, // used as the type of basic blocks and functions
        kVoid,
        kI1,
        kI8,
        kI16,
        kI32,
        kI64,
        kPointer, // opaque pointer type
        kArray
    };

    virtual ~Type() = default;

    constexpr ID id() const noexcept { return kind_; }

    virtual std::unique_ptr<Type> clone() const = 0;
    virtual bool is_equal(const Type &other) const = 0;
    virtual std::string to_string() const = 0;

  protected:
    Type(ID kind) noexcept : kind_{kind} {}

    Type(const Type &) = default;
    Type(Type &&) = default;

    static ID check_if_integral(ID kind);

  private:
    ID kind_;
};

constexpr std::string_view to_string_view(Type::ID type) {
    using namespace std::string_view_literals;
    using enum Type::ID;
    switch (type) {
    case kNone:
        return "none"sv;
    case kVoid:
        return "void"sv;
    case kI1:
        return "i1"sv;
    case kI8:
        return "i8"sv;
    case kI16:
        return "i16"sv;
    case kI32:
        return "i32"sv;
    case kI64:
        return "i64"sv;
    case kPointer:
        return "ptr"sv;
    case kArray:
        return "array"sv;
    default:
        std::unreachable();
    }
}

} // namespace bjac

namespace std {

template <>
struct formatter<::bjac::Type::ID> : public formatter<string_view> {
    using formatter<string_view>::parse;

    template <class FmtContext>
    FmtContext::iterator format(::bjac::Type::ID kind, FmtContext &ctx) const {
        return formatter<string_view>::format(::bjac::to_string_view(kind), ctx);
    }
};

} // namespace std

namespace bjac {

inline auto Type::check_if_integral(ID kind) -> ID {
    using enum ID;
    switch (kind) {
    case kI1:
    case kI8:
    case kI16:
    case kI32:
    case kI64:
        return kind;
    default:
        throw std::invalid_argument{std::format("'{}' is not an integral type", kind)};
    }
}

class NoneType final : public Type {
  public:
    NoneType() : Type(ID::kNone) {}

    std::unique_ptr<Type> clone() const override { return std::make_unique<NoneType>(); }
    bool is_equal(const Type &other) const override { return other.id() == ID::kNone; }
    std::string to_string() const override { return std::string{to_string_view(ID::kNone)}; }
};

class VoidType final : public Type {
  public:
    VoidType() : Type(ID::kVoid) {}

    std::unique_ptr<Type> clone() const override { return std::make_unique<VoidType>(); }
    bool is_equal(const Type &other) const override { return other.id() == ID::kVoid; }
    std::string to_string() const override { return std::string{to_string_view(ID::kVoid)}; }
};

class IntegralType final : public Type {
  public:
    explicit IntegralType(ID kind) : Type{check_if_integral(kind)} {}

    std::unique_ptr<Type> clone() const override {
        return std::unique_ptr<IntegralType>{new IntegralType{*this}};
    }

    bool is_equal(const Type &other) const override { return id() == other.id(); }
    std::string to_string() const override { return std::string{to_string_view(id())}; }
};

class PointerType final : public Type {
  public:
    explicit PointerType(ID referenced_kind)
        : Type{ID::kPointer}, referenced_kind_{check_if_integral(referenced_kind)} {}

    ID referenced_kind() const noexcept { return referenced_kind_; }

    std::unique_ptr<Type> clone() const override {
        return std::unique_ptr<PointerType>{new PointerType{*this}};
    }

    bool is_equal(const Type &other) const override {
        return other.id() == ID::kPointer &&
               referenced_kind_ == static_cast<const PointerType &>(other).referenced_kind_;
    }

    std::string to_string() const override { return std::string{to_string_view(ID::kPointer)}; }

  private:
    ID referenced_kind_;
};

class ArrayType final : public Type {
  public:
    explicit ArrayType(ID referenced_kind, std::size_t size)
        : Type{ID::kArray}, referenced_kind_{referenced_kind}, size_{size} {}

    ID referenced_kind() const noexcept { return referenced_kind_; }
    std::size_t size() const noexcept { return size_; }

    std::unique_ptr<Type> clone() const override {
        return std::unique_ptr<ArrayType>{new ArrayType{*this}};
    }

    bool is_equal(const Type &other) const override {
        return other.id() == ID::kArray && size_ == static_cast<const ArrayType &>(other).size_ &&
               referenced_kind_ == static_cast<const ArrayType &>(other).referenced_kind_;
    }

    std::string to_string() const override {
        return std::format("[{} x {}]", size_, referenced_kind_);
    }

  private:
    ID check_if_suitable_for_array(ID kind) {
        using enum ID;
        switch (kind) {
        case kI1:
        case kI8:
        case kI16:
        case kI32:
        case kI64:
        case kPointer:
            return kind;
        default:
            throw std::invalid_argument{
                std::format("'{}' is not suitable for placing in an array", kind)};
        }
    }

    ID referenced_kind_;
    std::size_t size_;
};

template <Type::ID kind>
struct BitWidth;

template <>
struct BitWidth<Type::ID::kI1> {
    static constexpr unsigned value = 1u;
};

template <>
struct BitWidth<Type::ID::kI8> {
    static constexpr unsigned value = 8u;
};

template <>
struct BitWidth<Type::ID::kI16> {
    static constexpr unsigned value = 16u;
};

template <>
struct BitWidth<Type::ID::kI32> {
    static constexpr unsigned value = 32u;
};

template <>
struct BitWidth<Type::ID::kI64> {
    static constexpr unsigned value = 64u;
};

template <>
struct BitWidth<Type::ID::kPointer> {
    static constexpr unsigned value = 64u;
};

template <Type::ID kind>
inline constexpr unsigned bit_width_v = BitWidth<kind>::value;

inline unsigned bit_width(const Type &type) {
    using enum Type::ID;

    auto scalar_type_bit_width = [](Type::ID kind) static {
        switch (kind) {
        case kI1:
            return bit_width_v<kI1>;
        case kI8:
            return bit_width_v<kI8>;
        case kI16:
            return bit_width_v<kI16>;
        case kI32:
            return bit_width_v<kI32>;
        case kI64:
            return bit_width_v<kI64>;
        case kPointer:
            return bit_width_v<kPointer>;
        default:
            std::unreachable();
        }
    };

    switch (type.id()) {
    case kNone:
    case kVoid:
        return 0;
    case kI1:
        return bit_width_v<kI1>;
    case kI8:
        return bit_width_v<kI8>;
    case kI16:
        return bit_width_v<kI16>;
    case kI32:
        return bit_width_v<kI32>;
    case kI64:
        return bit_width_v<kI64>;
    case kPointer:
        return bit_width_v<kPointer>;
    case kArray: {
        const auto &array_type = static_cast<const ArrayType &>(type);
        return scalar_type_bit_width(array_type.referenced_kind()) * array_type.size();
    }
    default:
        std::unreachable();
    }
}

} // namespace bjac

#endif // INCLUDE_BJAC_IR_TYPE_HPP
