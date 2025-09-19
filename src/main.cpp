#include "bjac/IR/instruction.hpp"

int main() {
    bjac::Instruction instr{bjac::Instruction::OtherOps::kCall};
    return instr.is_terminator();
}
