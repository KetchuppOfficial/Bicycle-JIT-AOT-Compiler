#ifndef INCLUDE_BJAC_IR_BINARY_OPERATOR_HPP
#define INCLUDE_BJAC_IR_BINARY_OPERATOR_HPP

#include <format>
#include <memory>
#include <stdexcept>
#include <string>

#include "bjac/IR/instruction.hpp"
#include "bjac/IR/type.hpp"
#include "bjac/IR/value.hpp"

namespace bjac {

class InvalidBinaryOperatorOpcode final : public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
};

class BinaryOperator final : public Instruction {
  public:
    static std::unique_ptr<BinaryOperator> create(Opcode opcode, Instruction &lhs,
                                                  Instruction &rhs) {
        return std::unique_ptr<BinaryOperator>{new BinaryOperator{opcode, lhs, rhs}};
    }

    ~BinaryOperator() override = default;

    template<typename Self>
    auto *get_lhs(this Self &&self) noexcept {
        return std::addressof(std::forward_like<Self>(*self.lhs_));
    }

    template<typename Self>
    auto *get_rhs(this Self &&self) noexcept {
        return std::addressof(std::forward_like<Self>(*self.rhs_));
    }

    std::string to_string() const override {
        return std::format("%{} = {} {} %{}, %{}", Value::to_void_ptr(this), type_, opcode_,
                           Value::to_void_ptr(lhs_), Value::to_void_ptr(rhs_));
    }

  private:
    BinaryOperator(Opcode opcode, Instruction &lhs, Instruction &rhs)
        : Instruction(check_opcode(opcode), common_type(lhs, rhs)), lhs_{&lhs}, rhs_{&rhs} {
        lhs.add_user(this);
        rhs.add_user(this);
    }

    static Opcode check_opcode(Opcode opcode) {
        if (opcode < Opcode::kBinaryBegin || opcode >= Opcode::kBinaryEnd) {
            throw InvalidBinaryOperatorOpcode{"invalid opcode for a binary operator"};
        }
        return opcode;
    }

    Type common_type(Instruction &lhs, Instruction &rhs) {
        const auto lhs_type = lhs.get_type();
        const auto rhs_type = rhs.get_type();
        if (lhs_type == rhs_type) {
            return lhs_type;
        } else {
            throw OperandsTypeMismatch{opcode_, lhs_type, rhs_type};
        }
    }

    Instruction *lhs_;
    Instruction *rhs_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_BINARY_OPERATOR_HPP
