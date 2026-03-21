#ifndef INCLUDE_BJAC_IR_CALL_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_CALL_INSTRUCTION_HPP

#include <string>
#include <vector>

#include "bjac/IR/instruction.hpp"

namespace bjac {

class Function;

class CallInstruction final : public Instruction {
  public:
    ~CallInstruction() override = default;

    Function &callee() noexcept { return *callee_; }
    const Function &callee() const noexcept { return *callee_; }

    Function &caller() noexcept;
    const Function &caller() const noexcept;

    std::ranges::view auto arguments() { return std::ranges::subrange{args_}; }
    std::ranges::view auto arguments() const {
        return args_ | std::views::transform(
                           [](Instruction *arg) static -> const Instruction * { return arg; });
    }

    bool is_recursive() const noexcept {
        return std::addressof(caller()) == std::addressof(callee());
    }

    std::string to_string() const override;

  private:
    friend class BasicBlock;

    CallInstruction(BasicBlock &parent, Function &callee, std::vector<Instruction *> args = {});

    virtual void remove_as_user() override { /* no-op */ }

    Function *callee_;
    std::vector<Instruction *> args_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_CALL_INSTRUCTION_HPP
