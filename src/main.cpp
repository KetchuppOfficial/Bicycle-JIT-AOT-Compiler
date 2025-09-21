#include <print>

#include "bjac/IR/instruction.hpp"

int main() {
    bjac::Instruction instr{bjac::Instruction::Opcode::kCall};
    std::println("{}", instr.get_name());
}
