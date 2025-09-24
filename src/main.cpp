#include <print>

#include "bjac/IR/instruction.hpp"
#include "bjac/utils/ilist.hpp"

int main() {
    bjac::ilist<bjac::Instruction> list;

    list.emplace_back(bjac::Instruction::Opcode::kCall);
    list.emplace_back(bjac::Instruction::Opcode::kAdd);
    list.emplace_back(bjac::Instruction::Opcode::kBr);
    list.emplace_back(bjac::Instruction::Opcode::kPHI);
    list.emplace_back(bjac::Instruction::Opcode::kRet);

    list.erase(std::next(list.cbegin(), 2)); // remove br
    list.pop_front(); // remove call
    list.pop_back(); // remove ret

    for (auto &instr : std::as_const(list)) { // add, phi
        std::println("{}", instr.get_name());
    }
}
