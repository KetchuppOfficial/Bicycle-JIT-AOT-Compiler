#ifndef INCLUDE_BJAC_IR_BOUNDS_CHECK_HPP
#define INCLUDE_BJAC_IR_BOUNDS_CHECK_HPP

#include "bjac/IR/instruction.hpp"

namespace bjac {

class BoundsCheckInstruction : public Instruction {
  public:
    ~BoundsCheckInstruction() override = default;

    Instruction *get_array() noexcept { return array_; }
    const Instruction *get_array() const noexcept { return array_; }

    Instruction *get_index() noexcept { return index_; }
    const Instruction *get_index() const noexcept { return index_; }

    std::vector<Instruction *> inputs() override { return {array_, index_}; }
    std::vector<const Instruction *> inputs() const override { return {array_, index_}; }

    std::string to_string() const override;

  private:
    friend class BasicBlock;

    BoundsCheckInstruction(BasicBlock &parent, Instruction &array, Instruction &index);

    void remove_as_user() override {
        array_->remove_user(this);
        index_->remove_user(this);
    }

    Instruction *array_;
    Instruction *index_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_BOUNDS_CHECK_HPP
