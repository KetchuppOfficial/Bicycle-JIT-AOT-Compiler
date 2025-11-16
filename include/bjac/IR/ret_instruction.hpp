#ifndef INCLUDE_BJAC_IR_RET_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_RET_INSTRUCTION_HPP

#include <memory>
#include <string>

#include "bjac/IR/instruction.hpp"
#include "bjac/IR/type.hpp"

namespace bjac {

class ReturnInstruction final : public Instruction {
  public:
    ~ReturnInstruction() override = default;

    Type get_ret_type() const noexcept { return ret_val_ ? ret_val_->get_type() : Type::kVoid; }

    Instruction *get_ret_value() noexcept { return ret_val_; }
    const Instruction *get_ret_value() const noexcept { return ret_val_; }
    void set_ret_value(Instruction &ret_val) noexcept { ret_val_ = std::addressof(ret_val); }

    std::string to_string() const override;

  private:
    friend class BasicBlock;

    ReturnInstruction(BasicBlock &parent); // ret void
    ReturnInstruction(BasicBlock &parent, Instruction &ret_val);

    Instruction *ret_val_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_RET_INSTRUCTION_HPP
