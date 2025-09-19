#ifndef INCLUDE_BJAC_IR_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_INSTRUCTION_HPP

#include <type_traits>
#include <utility>

namespace bjac {

namespace detail {

template <typename E>
concept InstructionEnum = std::is_scoped_enum_v<E> && requires {
    E::kBegin;
    E::kEnd;
};

} // namespace detail

class Instruction final {
  public:
    using opcode_type = unsigned char;

    enum class BinaryOps : opcode_type {
#define FIRST_BINARY_INSTR(N) kBegin = N,
#define HANDLE_BINARY_INSTR(N, Opcode, Class) k##Opcode = N,
#define LAST_BINARY_INSTR(N) kEnd = N + 1
#include "bjac/IR/instructions.def"
    };

    enum class TerminatorOps : opcode_type {
#define FIRST_TERMINATOR_INSTR(N) kBegin = N,
#define HANDLE_TERMINATOR_INSTR(N, Opcode, Class) k##Opcode = N,
#define LAST_TERMINATOR_INSTR(N) kEnd = N + 1
#include "bjac/IR/instructions.def"
    };

    enum class CastOps : opcode_type {
#define FIRST_CAST_INSTR(N) kBegin = N,
#define HANDLE_CAST_INSTR(N, Opcode, Class) k##Opcode = N,
#define LAST_CAST_INSTR(N) kEnd = N + 1
#include "bjac/IR/instructions.def"
    };

    enum class OtherOps : opcode_type {
#define FIRST_OTHER_INSTR(N) kBegin = N,
#define HANDLE_OTHER_INSTR(N, Opcode, Class) k##Opcode = N,
#define LAST_OTHER_INSTR(N) kEnd = N + 1
#include "bjac/IR/instructions.def"
    };

    template <detail::InstructionEnum E>
    explicit Instruction(E opcode) : opcode_{std::to_underlying(opcode)} {}

    opcode_type get_opcode() const noexcept { return opcode_; }

    static bool is_binary_op(opcode_type opcode) noexcept {
        return is_in_category<BinaryOps>(opcode);
    }

    bool is_binary_op() const noexcept { return Instruction::is_binary_op(get_opcode()); }

    static bool is_terminator(opcode_type opcode) noexcept {
        return is_in_category<TerminatorOps>(opcode);
    }

    bool is_terminator() const noexcept { return Instruction::is_terminator(get_opcode()); }

  private:
    template <detail::InstructionEnum E>
    static bool is_in_category(opcode_type opcode) noexcept {
        return std::to_underlying(E::kBegin) <= opcode && opcode < std::to_underlying(E::kEnd);
    }

    opcode_type opcode_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_INSTRUCTION_HPP
