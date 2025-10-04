#ifndef INCLUDE_BJAC_IR_PHI_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_PHI_INSTRUCTION_HPP

#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

#include "bjac/IR/instruction.hpp"
#include "bjac/IR/type.hpp"
#include "bjac/IR/value.hpp"

namespace bjac {

class PHITypeMismatch final : public std::invalid_argument {
    using std::invalid_argument::invalid_argument;
};

class PHIInstruction final : public Instruction {
  public:
    static std::unique_ptr<PHIInstruction> create(Type type) {
        return std::unique_ptr<PHIInstruction>{new PHIInstruction{type}};
    }

    ~PHIInstruction() override = default;

    void add_path(BasicBlock &bb, Value &value) {
        if (value.get_type() != type_) {
            throw PHITypeMismatch{"adding path of different type to a phi instruction"};
        }
        records_.emplace(std::addressof(bb), std::addressof(value));
    }
    void remove_path(BasicBlock &bb) { records_.erase(std::addressof(bb)); }

    template <typename Self>
    auto get(this Self &&self, BasicBlock &bb)
        -> std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const Value *,
                              Value *> {
        if (auto it = self.records_.find(std::addressof(bb)); it != self.records_.end()) {
            return it->second;
        }
        return nullptr;
    }

    std::string to_string() const override;

  protected:
    PHIInstruction(Type type) : Instruction(Opcode::kPHI, type) {}

  private:
    std::map<BasicBlock *, Value *> records_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_PHI_INSTRUCTION_HPP
