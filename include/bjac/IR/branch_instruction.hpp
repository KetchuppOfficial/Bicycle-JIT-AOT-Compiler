#ifndef INCLUDE_BJAC_IR_BRANCH_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_BRANCH_INSTRUCTION_HPP

#include <array>
#include <iterator>
#include <memory>
#include <ranges>
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

    Instruction *get_condition() noexcept { return condition_; }
    const Instruction *get_condition() const noexcept { return condition_; }
    void set_condition(Instruction &cond) noexcept { condition_ = check_condition(cond); }

    template <typename Self>
    auto *get_true_path(this Self &&self) noexcept {
        return std::addressof(std::forward_like<Self>(*self.paths_[0]));
    }

    void set_true_path(BasicBlock &bb) noexcept { paths_[0] = std::addressof(bb); }

    BasicBlock *get_false_path() noexcept { return paths_[1]; }
    const BasicBlock *get_false_path() const noexcept { return paths_[1]; }
    void set_false_path(BasicBlock &bb) noexcept { paths_[1] = std::addressof(bb); }

    auto successors() {
        return std::ranges::subrange{paths_.begin(),
                                     condition_ ? paths_.end() : std::ranges::next(paths_.begin())};
    }

    auto successors() const {
        auto succ = std::ranges::subrange{
            paths_.begin(), condition_ ? paths_.end() : std::ranges::next(paths_.begin())};

        return succ | std::views::transform(
                          [](BasicBlock *bb) static -> const BasicBlock * { return bb; });
    }

    std::string to_string() const override;

  private:
    friend class BasicBlock;

    BranchInstruction(BasicBlock &parent, BasicBlock &true_path)
        : Instruction(parent, Opcode::kBr, Type::kVoid), condition_{nullptr},
          paths_{std::addressof(true_path), nullptr} {}

    BranchInstruction(BasicBlock &parent, Instruction &condition, BasicBlock &true_path,
                      BasicBlock &false_path)
        : Instruction(parent, Opcode::kBr, Type::kVoid), condition_{check_condition(condition)},
          paths_{std::addressof(true_path), std::addressof(false_path)} {
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
