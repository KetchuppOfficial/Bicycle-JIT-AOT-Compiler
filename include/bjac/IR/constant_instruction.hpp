#ifndef INCLUDE_BJAC_IR_CONSTANT_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_CONSTANT_INSTRUCTION_HPP

#include <cstdint>
#include <format>
#include <memory>
#include <stdexcept>
#include <string>

#include "bjac/IR/instruction.hpp"
#include "bjac/IR/type.hpp"
#include "bjac/IR/value.hpp"

namespace bjac {

class InvalidConstantType final : public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
};

class ConstInstruction final : public Instruction {
  public:
    static std::unique_ptr<ConstInstruction> create(Type type, std::uintmax_t value) {
        return std::unique_ptr<ConstInstruction>{new ConstInstruction{type, value}};
    }

    ~ConstInstruction() = default;

    std::uintmax_t get_value() const noexcept { return value_; }

    std::string to_string() const override {
        return std::format("%{} = {} {} {}", Value::to_void_ptr(this), opcode_, type_, value_);
    }

  protected:
    ConstInstruction(Type type, std::uintmax_t value)
        : Instruction(Opcode::kConst, type), value_{value} {
        switch (type) {
        case Type::kVoid:
            throw InvalidConstantType{"void constant shall not be created"};
        case Type::kNone:
            throw InvalidConstantType{"none constant shall not be created"};
        default:
            break;
        }
    }

  private:
    std::uintmax_t value_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_CONSTANT_INSTRUCTION_HPP
