#ifndef INCLUDE_BJAC_IR_NULL_CHECK_HPP
#define INCLUDE_BJAC_IR_NULL_CHECK_HPP

#include "bjac/IR/instruction.hpp"

namespace bjac {

class NullCheckInstruction final : public Instruction {
  public:
    ~NullCheckInstruction() override = default;

    Instruction *get_input() noexcept { return input_; }
    const Instruction *get_input() const noexcept { return input_; }

    std::vector<Instruction *> inputs() override { return {input_}; }
    std::vector<const Instruction *> inputs() const override { return {input_}; }

    std::string to_string() const override;

  private:
    friend class BasicBlock;

    NullCheckInstruction(BasicBlock &parent, Instruction &input);

    void remove_as_user() override { input_->remove_user(this); }

    Instruction *input_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_NULL_CHECK_HPP
