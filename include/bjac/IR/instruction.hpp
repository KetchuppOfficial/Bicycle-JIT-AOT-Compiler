#ifndef INCLUDE_BJAC_IR_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_INSTRUCTION_HPP

#include <format>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "bjac/IR/type.hpp"
#include "bjac/IR/value.hpp"
#include "bjac/utils/ilist_node.hpp"

namespace bjac {

class BasicBlock;

class Instruction : public Value, public ilist_node<Instruction> {
    using node_type = ilist_node<Instruction>;

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

    ~Instruction() override = default;

    Opcode get_opcode() const noexcept { return opcode_; }

    template <typename Self>
    auto *get_parent(this Self &&self) noexcept {
        return std::addressof(std::forward_like<Self>(*self.parent_));
    }

    std::optional<unsigned> get_id() const { return parent_ ? std::optional{id_} : std::nullopt; }

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

    void add_user(const Instruction *value) { users_.insert(value); }
    void remove_user(const Instruction *value) { users_.erase(value); }

    virtual std::string to_string() const = 0;

  private:
    friend class BasicBlock;

    template <Opcode kBegin, Opcode kEnd>
    static constexpr bool is_in_category(Opcode opcode) noexcept {
        auto opc = std::to_underlying(opcode);
        return std::to_underlying(kBegin) <= opc && opc < std::to_underlying(kEnd);
    }

  protected:
    Instruction(BasicBlock &parent, Opcode opcode, Type type) noexcept;

    Opcode opcode_;
    BasicBlock *parent_;
    unsigned id_;
    std::set<const Instruction *> users_;
};

inline std::string_view to_string_view(Instruction::Opcode opcode) noexcept {
    using namespace std::string_view_literals;
    switch (opcode) {
    default:
        std::unreachable();
#define HANDLE_INSTR(N, Opc, Class, Name)                                                          \
    case Instruction::Opcode::k##Opc:                                                              \
        return #Name##sv;
#include "bjac/IR/instructions.def"
    }
}

} // namespace bjac

namespace std {

template <>
struct formatter<::bjac::Instruction::Opcode> final : public formatter<string_view> {
    using formatter<string_view>::parse;

    template <class FmtContext>
    FmtContext::iterator format(::bjac::Instruction::Opcode opcode, FmtContext &ctx) const {
        return formatter<string_view>::format(::bjac::to_string_view(opcode), ctx);
    }
};

} // namespace std

namespace bjac {

class OperandsTypeMismatch final : public std::invalid_argument {
  public:
    OperandsTypeMismatch(Instruction::Opcode opcode, Type type_1, Type type_2)
        : std::invalid_argument{
              std::format("operands of {} instruction are of different types: {} and {}", opcode,
                          type_1, type_2)} {}
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_INSTRUCTION_HPP
