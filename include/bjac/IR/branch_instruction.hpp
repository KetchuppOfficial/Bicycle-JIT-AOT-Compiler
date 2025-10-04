#ifndef INCLUDE_BJAC_IR_BRANCH_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_BRANCH_INSTRUCTION_HPP

#include <memory>
#include <stdexcept>
#include <string>

#include "bjac/IR/instruction.hpp"
#include "bjac/IR/type.hpp"
#include "bjac/IR/value.hpp"

namespace bjac {

class BasicBlock;

class InvalidConditionType final : public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
};

class BranchInstruction final : public Instruction {
  public:
    static std::unique_ptr<BranchInstruction> create(BasicBlock &true_path) {
        return std::unique_ptr<BranchInstruction>{new BranchInstruction{true_path}};
    }

    static std::unique_ptr<BranchInstruction> create(Value &condition, BasicBlock &true_path,
                                                     BasicBlock &false_path) {
        return std::unique_ptr<BranchInstruction>{
            new BranchInstruction{condition, true_path, false_path}};
    }

    ~BranchInstruction() override = default;

    bool is_conditional() const noexcept { return condition_ != nullptr; }

    Value *get_condition() noexcept { return condition_; }
    const Value *get_condition() const noexcept { return condition_; }

    BasicBlock *get_true_path() noexcept { return true_path_; }
    const BasicBlock *get_true_path() const noexcept { return true_path_; }
    void set_true_path(BasicBlock &bb) noexcept { true_path_ = &bb; }

    BasicBlock *get_false_path() noexcept { return false_path_; }
    const BasicBlock *get_false_path() const noexcept { return false_path_; }
    void set_false_path(BasicBlock &bb) noexcept { false_path_ = &bb; }

    std::string to_string() const override;

  private:
    BranchInstruction(BasicBlock &true_path)
        : Instruction(Opcode::kBr, Type::kVoid), condition_{nullptr}, true_path_{&true_path},
          false_path_{nullptr} {}

    BranchInstruction(Value &condition, BasicBlock &true_path, BasicBlock &false_path)
        : Instruction(Opcode::kBr, Type::kVoid), condition_{check_condition(condition)},
          true_path_{&true_path}, false_path_{&false_path} {}

    static Value *check_condition(Value &cond) {
        if (cond.get_type() != Type::kI1) {
            throw InvalidConditionType{"type of condition is not i1"};
        }
        return &cond;
    }

    Value *condition_;
    BasicBlock *true_path_;
    BasicBlock *false_path_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_BRANCH_INSTRUCTION_HPP
