#ifndef INCLUDE_BJAC_IR_BRANCH_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_BRANCH_INSTRUCTION_HPP

#include <array>
#include <iterator>
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
        return std::addressof(std::forward_like<Self>(*self.paths_[0]));
    }

    void set_true_path(BasicBlock &bb) noexcept { paths_[0] = &bb; }

    template <typename Self>
    auto *get_false_path(this Self &&self) noexcept {
        return std::addressof(std::forward_like<Self>(*self.paths_[1]));
    }

    void set_false_path(BasicBlock &bb) noexcept { paths_[1] = &bb; }

    auto successors() const {
        return std::ranges::subrange{paths_.begin(),
                                     condition_ ? paths_.end() : std::ranges::next(paths_.begin())};
    }

    std::string to_string() const override;

  private:
    friend class BasicBlock;

    BranchInstruction(BasicBlock &parent, BasicBlock &true_path)
        : Instruction(parent, Opcode::kBr, Type::kVoid), condition_{nullptr},
          paths_{&true_path, nullptr} {}

    BranchInstruction(BasicBlock &parent, Instruction &condition, BasicBlock &true_path,
                      BasicBlock &false_path)
        : Instruction(parent, Opcode::kBr, Type::kVoid), condition_{check_condition(condition)},
          paths_{&true_path, &false_path} {
        condition.add_user(this);
    }

    static Instruction *check_condition(Instruction &cond) {
        if (cond.get_type() != Type::kI1) {
            throw InvalidConditionType{"type of condition is not i1"};
        }
        return std::addressof(cond);
    }

    Instruction *condition_;
    std::array<BasicBlock *, 2> paths_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_BRANCH_INSTRUCTION_HPP
