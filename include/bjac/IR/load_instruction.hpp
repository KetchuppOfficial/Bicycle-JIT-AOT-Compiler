#ifndef INCLUDE_BJAC_IR_LOAD_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_LOAD_INSTRUCTION_HPP

#include "bjac/IR/instruction.hpp"

namespace bjac {

class LoadInstruction final : public Instruction {
  public:
    ~LoadInstruction() override = default;

    Instruction *get_addr() noexcept { return addr_; }
    const Instruction *get_addr() const noexcept { return addr_; }

    std::vector<Instruction *> inputs() override { return {addr_}; }
    std::vector<const Instruction *> inputs() const override { return {addr_}; }

    std::string to_string() const override;

  private:
    friend class BasicBlock;

    LoadInstruction(BasicBlock &parent, std::unique_ptr<Type> type, Instruction &addr);

    void remove_as_user() override { addr_->remove_user(this); }

    Instruction *addr_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_LOAD_INSTRUCTION_HPP
