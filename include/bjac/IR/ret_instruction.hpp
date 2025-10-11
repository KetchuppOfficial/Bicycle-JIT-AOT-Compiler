#ifndef INCLUDE_BJAC_IR_RET_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_RET_INSTRUCTION_HPP

#include <format>
#include <memory>
#include <string>

#include "bjac/IR/instruction.hpp"
#include "bjac/IR/type.hpp"
#include "bjac/IR/value.hpp"

namespace bjac {

class ReturnInstruction final : public Instruction {
  public:
    static std::unique_ptr<ReturnInstruction> create() {
        return std::unique_ptr<ReturnInstruction>{new ReturnInstruction{}};
    }

    static std::unique_ptr<ReturnInstruction> create(Instruction &ret_val) {
        return std::unique_ptr<ReturnInstruction>{new ReturnInstruction{ret_val}};
    }

    ~ReturnInstruction() override = default;

    Type get_ret_type() const noexcept { return ret_val_ ? ret_val_->get_type() : Type::kVoid; }

    template <typename Self>
    auto *get_ret_value(this Self &&self) noexcept {
        return std::addressof(std::forward_like<Self>(*self.ret_val_));
    }

    std::string to_string() const override {
        if (ret_val_) {
            return std::format("{} {} %{}", opcode_, ret_val_->get_type(),
                               Value::to_void_ptr(ret_val_));
        }
        return std::format("{} {}", opcode_, Type::kVoid);
    }

  private:
    ReturnInstruction() : Instruction(Opcode::kRet, Type::kVoid), ret_val_{nullptr} {} // ret void
    ReturnInstruction(Instruction &ret_val)
        : Instruction(Opcode::kRet, Type::kVoid), ret_val_{&ret_val} {
        ret_val.add_user(this);
    }

    Instruction *ret_val_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_RET_INSTRUCTION_HPP
