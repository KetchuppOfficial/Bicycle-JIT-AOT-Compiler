#ifndef INCLUDE_BJAC_IR_PHI_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_PHI_INSTRUCTION_HPP

#include <map>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>

#include "bjac/IR/instruction.hpp"
#include "bjac/IR/type.hpp"

namespace bjac {

class PHITypeMismatch final : public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
};

class PHIInstruction final : public Instruction {
  public:
    ~PHIInstruction() override = default;

    void add_path(BasicBlock &bb, Instruction &value) {
        if (value.get_type() != type_) {
            throw PHITypeMismatch{"adding path of different type to a phi instruction"};
        }
        records_.emplace(std::addressof(bb), std::addressof(value));
        value.add_user(this);
    }

    void remove_path(BasicBlock &bb) {
        if (auto it = records_.find(std::addressof(bb)); it != records_.end()) {
            it->second->remove_user(this);
            records_.erase(it);
        }
    }

    void replace_value(Instruction &from, Instruction &to) {
        for (auto &instr : records_ | std::views::values) {
            if (instr == std::addressof(from)) {
                instr->remove_user(this);
                instr = std::addressof(to);
                instr->add_user(this);
            }
        }
    }

    template <typename Self>
    auto *get_value(this Self &&self, BasicBlock &bb) {
        if (auto it = self.records_.find(std::addressof(bb)); it != self.records_.end()) {
            return std::addressof(std::forward_like<Self>(*it->second));
        }
        return nullptr;
    }

    std::ranges::bidirectional_range auto get_sources() { return records_ | std::views::keys; }

    std::ranges::bidirectional_range auto get_sources() const {
        return records_ | std::views::keys |
               std::views::transform(
                   [](BasicBlock *bb) static -> const BasicBlock * { return bb; });
    }

    std::ranges::bidirectional_range auto get_values() { return records_ | std::views::values; }

    std::ranges::bidirectional_range auto get_values() const {
        return records_ | std::views::values |
               std::views::transform(
                   [](Instruction *i) static -> const Instruction * { return i; });
    }

    std::string to_string() const override;

  private:
    friend class BasicBlock;

    PHIInstruction(BasicBlock &parent, Type type) : Instruction(parent, Opcode::kPHI, type) {}

    void remove_as_user() override {
        for (auto *value : records_ | std::views::values) {
            value->remove_user(this);
        }
    }

    std::map<BasicBlock *, Instruction *> records_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_PHI_INSTRUCTION_HPP
