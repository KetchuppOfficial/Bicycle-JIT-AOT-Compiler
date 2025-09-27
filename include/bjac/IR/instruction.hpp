#ifndef INCLUDE_BJAC_IR_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_INSTRUCTION_HPP

#include <string_view>
#include <utility>

#include "bjac/utils/ilist_node.hpp"

namespace bjac {

class BasicBlock;

class Instruction final : public ilist_node<Instruction> {
  public:
    enum class Opcode : unsigned char {
#define FIRST_BINARY_INSTR(N) kBinaryBegin = N,
#define HANDLE_BINARY_INSTR(N, Opcode, Class, Name) k##Opcode = N,
#define LAST_BINARY_INSTR(N) kBinaryEnd = N + 1,

#define FIRST_TERMINATOR_INSTR(N) kTerminatorsBegin = N,
#define HANDLE_TERMINATOR_INSTR(N, Opcode, Class, Name) k##Opcode = N,
#define LAST_TERMINATOR_INSTR(N) kTerminatorsEnd = N + 1,

#define FIRST_CAST_INSTR(N) kCastsBegin = N,
#define HANDLE_CAST_INSTR(N, Opcode, Class, Name) k##Opcode = N,
#define LAST_CAST_INSTR(N) kCastsEnd = N + 1,

#define FIRST_OTHER_INSTR(N) kOtherBegin = N,
#define HANDLE_OTHER_INSTR(N, Opcode, Class, Name) k##Opcode = N,
#define LAST_OTHER_INSTR(N) kOtherEnd = N + 1,

#include "bjac/IR/instructions.def"
    };

    explicit Instruction(Opcode opcode) : opcode_{opcode} {}

    ~Instruction() override = default;

    Opcode get_opcode() const noexcept { return opcode_; }

    static constexpr bool is_binary_op(Opcode opcode) noexcept {
        return is_in_category<Opcode::kBinaryBegin, Opcode::kBinaryEnd>(opcode);
    }

    bool is_binary_op() const noexcept { return Instruction::is_binary_op(opcode_); }

    static constexpr bool is_terminator(Opcode opcode) noexcept {
        return is_in_category<Opcode::kTerminatorsBegin, Opcode::kTerminatorsEnd>(opcode);
    }

    bool is_terminator() const noexcept { return Instruction::is_terminator(opcode_); }

    static constexpr bool is_cast(Opcode opcode) noexcept {
        return is_in_category<Opcode::kCastsBegin, Opcode::kCastsEnd>(opcode);
    }

    bool is_cast() const noexcept { return Instruction::is_cast(opcode_); }

    static constexpr bool is_other_op(Opcode opcode) noexcept {
        return is_in_category<Opcode::kOtherBegin, Opcode::kOtherEnd>(opcode);
    }

    bool is_other_op() const noexcept { return Instruction::is_other_op(opcode_); }

    static std::string_view get_name(Opcode opcode) noexcept {
        using enum Opcode;
        using namespace std::string_view_literals;
        switch (opcode) {
#define HANDLE_INSTR(N, Opcode, Class, Name)                                                       \
    case k##Opcode:                                                                                \
        return #Name##sv;
#include "bjac/IR/instructions.def"

        default:
            std::unreachable();
        }
    }

    std::string_view get_name() const noexcept { return Instruction::get_name(opcode_); }

  private:
    template <Opcode kBegin, Opcode kEnd>
    static constexpr bool is_in_category(Opcode opcode) noexcept {
        auto opc = std::to_underlying(opcode);
        return std::to_underlying(kBegin) <= opc && opc < std::to_underlying(kEnd);
    }

    Opcode opcode_;
    BasicBlock *parent_ = nullptr;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_INSTRUCTION_HPP
