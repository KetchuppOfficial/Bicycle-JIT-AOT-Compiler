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
struct IRTypeToMaxValue<Type::ID::kI1> {
    static constexpr std::uintmax_t kMax = std::numeric_limits<bool>::max();
};

template <>
struct IRTypeToMaxValue<Type::ID::kI8> {
    static constexpr std::uintmax_t kMax = std::numeric_limits<std::uint8_t>::max();
};

template <>
struct IRTypeToMaxValue<Type::ID::kI16> {
    static constexpr std::uintmax_t kMax = std::numeric_limits<std::uint16_t>::max();
};

template <>
struct IRTypeToMaxValue<Type::ID::kI32> {
    static constexpr std::uintmax_t kMax = std::numeric_limits<std::uint32_t>::max();
};

template <>
struct IRTypeToMaxValue<Type::ID::kI64> {
    static constexpr std::uintmax_t kMax = std::numeric_limits<std::uint64_t>::max();
};

class ConstInstruction final : public Instruction {
  public:
    ~ConstInstruction() override = default;

    std::uintmax_t get_value() const noexcept { return value_; }

    std::uintmax_t max_value() const {
        using enum Type::ID;
        switch (get_type().id()) {
        case kI1:
            return IRTypeToMaxValue<kI1>::kMax;
        case kI8:
            return IRTypeToMaxValue<kI8>::kMax;
        case kI16:
            return IRTypeToMaxValue<kI16>::kMax;
        case kI32:
            return IRTypeToMaxValue<kI32>::kMax;
        case kI64:
            return IRTypeToMaxValue<kI64>::kMax;
        default:
            std::unreachable();
        }
    }

    std::string to_string() const override;

  private:
    friend class BasicBlock;

    ConstInstruction(BasicBlock &parent, std::unique_ptr<Type> type, std::uintmax_t value)
        : Instruction(parent, Opcode::kConst, std::move(type)), value_{value} {
        using enum Type::ID;
        switch (const auto id = get_type().id()) {
        case kI1:
            if (value > IRTypeToMaxValue<kI1>::kMax) {
                throw ConstantOutOfRange{"value for i1 is out of range"};
            }
            break;
        case kI8:
            if (value > IRTypeToMaxValue<kI8>::kMax) {
                throw ConstantOutOfRange{"value for i8 is out of range"};
            }
            break;
        case kI16:
            if (value > IRTypeToMaxValue<kI16>::kMax) {
                throw ConstantOutOfRange{"value for i16 is out of range"};
            }
            break;
        case kI32:
            if (value > IRTypeToMaxValue<kI32>::kMax) {
                throw ConstantOutOfRange{"value for i32 is out of range"};
            }
            break;
        case kI64:
            if (value > IRTypeToMaxValue<kI64>::kMax) {
                throw ConstantOutOfRange{"value for i64 is out of range"};
            }
            break;
        default:
            throw InvalidConstantType{std::format("'{}' constant shall not be created", id)};
        }
    }

    virtual void remove_as_user() override { /* no-op */ }

    std::uintmax_t value_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_CONSTANT_INSTRUCTION_HPP
