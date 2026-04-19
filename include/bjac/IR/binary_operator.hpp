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
        if (!get_type().is_equal(lhs.get_type())) {
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
        if (!get_type().is_equal(rhs.get_type())) {
            throw OperandsTypeMismatch{opcode_, get_type(), rhs.get_type()};
        }
        rhs_->remove_user(this);
        rhs_ = std::addressof(rhs);
        rhs_->add_user(this);
    }

    std::string to_string() const override;

    std::vector<Instruction *> inputs() override { return {lhs_, rhs_}; }
    std::vector<const Instruction *> inputs() const override { return {lhs_, rhs_}; }

  private:
    friend class BasicBlock;

    BinaryOperator(BasicBlock &parent, Opcode opcode, Instruction &lhs, Instruction &rhs)
        : Instruction(parent, check_opcode(opcode), common_type(opcode, lhs, rhs)),
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

    static std::unique_ptr<Type> common_type(Opcode opcode, const Instruction &lhs,
                                             const Instruction &rhs) {
        const auto &lhs_type = lhs.get_type();
        const auto &rhs_type = rhs.get_type();
        if (lhs_type.is_equal(rhs_type)) {
            return lhs_type.clone();
        } else {
            throw OperandsTypeMismatch{opcode, lhs_type, rhs_type};
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
