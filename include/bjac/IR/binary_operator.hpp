#ifndef INCLUDE_BJAC_IR_BINARY_OPERATOR_HPP
#define INCLUDE_BJAC_IR_BINARY_OPERATOR_HPP

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "bjac/IR/instruction.hpp"
#include "bjac/IR/type.hpp"

namespace bjac {

class InvalidBinaryOperatorOpcode final : public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
};

class BinaryOperator final : public Instruction {
  public:
    ~BinaryOperator() override = default;

    template <typename Self>
    auto *get_lhs(this Self &&self) noexcept {
        return std::addressof(std::forward_like<Self>(*self.lhs_));
    }

    void set_lhs(Instruction &lhs) {
        if (lhs.get_type() != get_type()) {
            throw OperandsTypeMismatch{opcode_, lhs.get_type(), get_type()};
        }
        lhs_->remove_user(this);
        lhs_ = std::addressof(lhs);
        lhs_->add_user(this);
    }

    template <typename Self>
    auto *get_rhs(this Self &&self) noexcept {
        return std::addressof(std::forward_like<Self>(*self.rhs_));
    }

    void set_rhs(Instruction &rhs) {
        if (rhs.get_type() != get_type()) {
            throw OperandsTypeMismatch{opcode_, get_type(), rhs.get_type()};
        }
        rhs_->remove_user(this);
        rhs_ = std::addressof(rhs);
        rhs_->add_user(this);
    }

    std::string to_string() const override;

  private:
    friend class BasicBlock;

    BinaryOperator(BasicBlock &parent, Opcode opcode, Instruction &lhs, Instruction &rhs)
        : Instruction(parent, check_opcode(opcode), common_type(lhs, rhs)),
          lhs_{std::addressof(lhs)}, rhs_{std::addressof(rhs)} {
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

    void remove_as_user() override {
        lhs_->remove_user(this);
        rhs_->remove_user(this);
    }

    Instruction *lhs_;
    Instruction *rhs_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_BINARY_OPERATOR_HPP
