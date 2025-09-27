#include <print>

#include "bjac/IR/basic_block.hpp"

int main() {
    bjac::BasicBlock bb;

    bb.emplace_back(bjac::Instruction::Opcode::kCall);
    bb.emplace_back(bjac::Instruction::Opcode::kAdd);
    bb.emplace_back(bjac::Instruction::Opcode::kBr);
    bb.emplace_back(bjac::Instruction::Opcode::kPHI);
    bb.emplace_back(bjac::Instruction::Opcode::kRet);

    bb.erase(std::next(bb.cbegin(), 2)); // remove br
    bb.pop_front(); // remove call
    bb.pop_back(); // remove ret

    for (auto &instr : std::as_const(bb)) { // add, phi
        std::println("{}", instr.get_name());
    }
}
