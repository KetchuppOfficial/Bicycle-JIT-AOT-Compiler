#ifndef INCLUDE_BJAC_IR_BRANCH_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_BRANCH_INSTRUCTION_HPP

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "bjac/IR/instruction.hpp"
#include "bjac/IR/type.hpp"

namespace bjac {

class BasicBlock;

class InvalidConditionType final : public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
};

class BranchInstruction final : public Instruction {
  public:
    ~BranchInstruction() override = default;

    bool is_conditional() const noexcept { return condition_ != nullptr; }

    template <typename Self>
    auto *get_condition(this Self &&self) noexcept {
        return std::addressof(std::forward_like<Self>(*self.condition_));
    }

    template <typename Self>
    auto *get_true_path(this Self &&self) noexcept {
        return std::addressof(std::forward_like<Self>(*self.true_path_));
    }

    void set_true_path(BasicBlock &bb) noexcept { true_path_ = &bb; }

    template <typename Self>
    auto *get_false_path(this Self &&self) noexcept {
        return std::addressof(std::forward_like<Self>(*self.false_path_));
    }

    void set_false_path(BasicBlock &bb) noexcept { false_path_ = &bb; }

    std::string to_string() const override;

  private:
    friend class BasicBlock;

    BranchInstruction(BasicBlock &parent, BasicBlock &true_path)
        : Instruction(parent, Opcode::kBr, Type::kVoid), condition_{nullptr},
          true_path_{&true_path}, false_path_{nullptr} {}

    BranchInstruction(BasicBlock &parent, Instruction &condition, BasicBlock &true_path,
                      BasicBlock &false_path)
        : Instruction(parent, Opcode::kBr, Type::kVoid), condition_{check_condition(condition)},
          true_path_{&true_path}, false_path_{&false_path} {
        condition.add_user(this);
    }

    static Instruction *check_condition(Instruction &cond) {
        if (cond.get_type() != Type::kI1) {
            throw InvalidConditionType{"type of condition is not i1"};
        }
        return std::addressof(cond);
    }

    Instruction *condition_;
    BasicBlock *true_path_;
    BasicBlock *false_path_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_BRANCH_INSTRUCTION_HPP
