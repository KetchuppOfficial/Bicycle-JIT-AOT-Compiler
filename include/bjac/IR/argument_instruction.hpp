#ifndef INCLUDE_BJAC_IR_ARGUMENT_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_ARGUMENT_INSTRUCTION_HPP

#include <stdexcept>
#include <string>

#include "bjac/IR/instruction.hpp"

namespace bjac {

class Function;

class ArgOutOfRange final : public std::out_of_range {
    using std::out_of_range::out_of_range;
};

class ArgumentInstruction final : public Instruction {
  public:
    ~ArgumentInstruction() override = default;

    unsigned get_position() const noexcept { return pos_; }

    std::string to_string() const override;

  private:
    friend class BasicBlock;

    ArgumentInstruction(BasicBlock &parent, const Function &f, unsigned pos);

    virtual void remove_as_user() override { /* no-op */ }

    unsigned pos_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_ARGUMENT_INSTRUCTION_HPP
