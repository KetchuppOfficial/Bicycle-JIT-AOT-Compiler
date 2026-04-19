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

    Type::ID get_ret_type_id() const noexcept {
        return ret_val_ ? ret_val_->get_type_id() : Type::ID::kVoid;
    }

    Instruction *get_ret_value() noexcept { return ret_val_; }
    const Instruction *get_ret_value() const noexcept { return ret_val_; }
    void set_ret_value(Instruction &ret_val) {
        const auto &new_ret_type = ret_val.get_type();
        if (ret_val_) {
            const auto &curr_ret_type = ret_val_->get_type();
            if (!new_ret_type.is_equal(curr_ret_type)) {
                throw std::invalid_argument{std::format("trying to change {} {} to {} {}",
                                                        Opcode::kRet, new_ret_type.to_string(),
                                                        Opcode::kRet, curr_ret_type.to_string())};
            }
        } else if (new_ret_type.id() != Type::ID::kVoid) {
            throw std::invalid_argument{std::format("trying to change {} {} to {} {}", Opcode::kRet,
                                                    new_ret_type.to_string(), Opcode::kRet,
                                                    Type::ID::kVoid)};
        }

        ret_val_->remove_user(this);
        ret_val_ = std::addressof(ret_val);
        ret_val_->add_user(this);
    }

    std::string to_string() const override;

    std::vector<Instruction *> inputs() override { return {ret_val_}; }
    std::vector<const Instruction *> inputs() const override { return {ret_val_}; }

  private:
    friend class BasicBlock;

    ReturnInstruction(BasicBlock &parent); // ret void
    ReturnInstruction(BasicBlock &parent, Instruction &ret_val);

    void remove_as_user() override { ret_val_->remove_user(this); }

    Instruction *ret_val_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_RET_INSTRUCTION_HPP
