#ifndef INCLUDE_BJAC_IR_ARGUMENT_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_ARGUMENT_INSTRUCTION_HPP

#include <format>
#include <memory>
#include <stdexcept>
#include <string>

#include "bjac/IR/instruction.hpp"
#include "bjac/IR/value.hpp"

namespace bjac {

class Function;

class ArgOutOfRange final : public std::out_of_range {
    using std::out_of_range::out_of_range;
};

class ArgumentInstruction final : public Instruction {
  public:
    static std::unique_ptr<ArgumentInstruction> create(const Function &f, unsigned pos) {
        return std::unique_ptr<ArgumentInstruction>{new ArgumentInstruction{f, pos}};
    }

    ~ArgumentInstruction() override = default;

    unsigned get_position() const noexcept { return pos_; }

    std::string to_string() const override {
        return std::format("%{} = {} {} [{}]", Value::to_void_ptr(this), opcode_, type_, pos_);
    }

  protected:
    ArgumentInstruction(const Function &f, unsigned pos);

  private:
    unsigned pos_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_ARGUMENT_INSTRUCTION_HPP
