#ifndef INCLUDE_BJAC_IR_RET_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_RET_INSTRUCTION_HPP

#include <memory>
#include <string>
#include <utility>

#include "bjac/IR/instruction.hpp"
#include "bjac/IR/type.hpp"

namespace bjac {

class ReturnInstruction final : public Instruction {
  public:
    ~ReturnInstruction() override = default;

    Type get_ret_type() const noexcept { return ret_val_ ? ret_val_->get_type() : Type::kVoid; }

    template <typename Self>
    auto *get_ret_value(this Self &&self) noexcept {
        return std::addressof(std::forward_like<Self>(*self.ret_val_));
    }

    std::string to_string() const override;

  private:
    friend class BasicBlock;

    ReturnInstruction(BasicBlock &parent)
        : Instruction(parent, Opcode::kRet, Type::kVoid), ret_val_{nullptr} {} // ret void
    ReturnInstruction(BasicBlock &parent, Instruction &ret_val)
        : Instruction(parent, Opcode::kRet, Type::kVoid), ret_val_{&ret_val} {
        ret_val.add_user(this);
    }

    Instruction *ret_val_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_RET_INSTRUCTION_HPP
