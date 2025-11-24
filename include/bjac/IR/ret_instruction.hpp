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
    void set_ret_value(Instruction &ret_val) {
        if (auto ret_type = get_ret_type(); ret_type != ret_val.get_type()) {
            throw std::invalid_argument{std::format("trying to change {} {} to {} {}", Opcode::kRet,
                                                    ret_val.get_type(), Opcode::kRet, ret_type)};
        }
        ret_val_->remove_user(this);
        ret_val_ = std::addressof(ret_val);
        ret_val_->add_user(this);
    }

    std::string to_string() const override;

  private:
    friend class BasicBlock;

    ReturnInstruction(BasicBlock &parent); // ret void
    ReturnInstruction(BasicBlock &parent, Instruction &ret_val);

    void remove_as_user() override { ret_val_->remove_user(this); }

    Instruction *ret_val_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_RET_INSTRUCTION_HPP
