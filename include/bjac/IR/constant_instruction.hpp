#ifndef INCLUDE_BJAC_IR_CONSTANT_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_CONSTANT_INSTRUCTION_HPP

#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

#include "bjac/IR/instruction.hpp"
#include "bjac/IR/type.hpp"

namespace bjac {

class InvalidConstantType final : public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
};

class ConstantOutOfRange final : public std::out_of_range {
    using std::out_of_range::out_of_range;
};

template <auto T>
struct IRTypeToMaxValue;

template <>
struct IRTypeToMaxValue<Type::kI1> {
    static constexpr std::uintmax_t kMax = std::numeric_limits<bool>::max();
};

template <>
struct IRTypeToMaxValue<Type::kI8> {
    static constexpr std::uintmax_t kMax = std::numeric_limits<std::uint8_t>::max();
};

template <>
struct IRTypeToMaxValue<Type::kI16> {
    static constexpr std::uintmax_t kMax = std::numeric_limits<std::uint16_t>::max();
};

template <>
struct IRTypeToMaxValue<Type::kI32> {
    static constexpr std::uintmax_t kMax = std::numeric_limits<std::uint32_t>::max();
};

template <>
struct IRTypeToMaxValue<Type::kI64> {
    static constexpr std::uintmax_t kMax = std::numeric_limits<std::uint64_t>::max();
};

class ConstInstruction final : public Instruction {
  public:
    ~ConstInstruction() = default;

    std::uintmax_t get_value() const noexcept { return value_; }

    std::string to_string() const override;

  private:
    friend class BasicBlock;

    ConstInstruction(BasicBlock &parent, Type type, std::uintmax_t value)
        : Instruction(parent, Opcode::kConst, type), value_{value} {
        switch (type) {
        case Type::kI1:
            if (value > IRTypeToMaxValue<Type::kI1>::kMax) {
                throw ConstantOutOfRange{"value for i1 is out of range"};
            }
            break;
        case Type::kI8:
            if (value > IRTypeToMaxValue<Type::kI8>::kMax) {
                throw ConstantOutOfRange{"value for i8 is out of range"};
            }
            break;
        case Type::kI16:
            if (value > IRTypeToMaxValue<Type::kI16>::kMax) {
                throw ConstantOutOfRange{"value for i16 is out of range"};
            }
            break;
        case Type::kI32:
            if (value > IRTypeToMaxValue<Type::kI32>::kMax) {
                throw ConstantOutOfRange{"value for i32 is out of range"};
            }
            break;
        case Type::kI64:
            if (value > IRTypeToMaxValue<Type::kI64>::kMax) {
                throw ConstantOutOfRange{"value for i64 is out of range"};
            }
            break;
        case Type::kVoid:
            throw InvalidConstantType{"void constant shall not be created"};
        case Type::kNone:
            throw InvalidConstantType{"none constant shall not be created"};
        default:
            std::unreachable();
        }
    }

    std::uintmax_t value_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_CONSTANT_INSTRUCTION_HPP
