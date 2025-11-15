#ifndef INCLUDE_BJAC_IR_ICMP_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_ICMP_INSTRUCTION_HPP

#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "bjac/IR/instruction.hpp"
#include "bjac/IR/type.hpp"

namespace bjac {

class ICmpInstruction final : public Instruction {
  public:
    enum class Kind { eq, ne, ugt, uge, ult, ule, sgt, sge, slt, sle };

    ~ICmpInstruction() override = default;

    Kind get_kind() const noexcept { return kind_; }

    template <typename Self>
    auto *get_lhs(this Self &&self) noexcept {
        return std::addressof(std::forward_like<Self>(*self.lhs_));
    }

    void set_lhs(Instruction &lhs) noexcept { lhs_ = &lhs; }

    template <typename Self>
    auto *get_rhs(this Self &&self) noexcept {
        return std::addressof(std::forward_like<Self>(*self.rhs_));
    }

    void set_rhs(Instruction &rhs) noexcept { rhs_ = &rhs; }

    std::string to_string() const override;

  private:
    friend class BasicBlock;

    ICmpInstruction(BasicBlock &parent, Kind kind, Instruction &lhs, Instruction &rhs)
        : Instruction(parent, Opcode::kICmp, Type::kI1), kind_{kind}, lhs_{&lhs}, rhs_{&rhs} {
        const auto lhs_type = lhs.get_type();
        const auto rhs_type = rhs.get_type();
        if (lhs_type != rhs_type) {
            throw OperandsTypeMismatch{opcode_, lhs_type, rhs_type};
        }

        lhs.add_user(this);
        rhs.add_user(this);
    }

    Kind kind_;
    Instruction *lhs_;
    Instruction *rhs_;
};

inline std::string_view to_string_view(ICmpInstruction::Kind kind) {
    using enum ICmpInstruction::Kind;
    switch (kind) {
    case eq:
        return "eq";
    case ne:
        return "ne";
    case ugt:
        return "ugt";
    case uge:
        return "uge";
    case ult:
        return "ult";
    case ule:
        return "ule";
    case sgt:
        return "sgt";
    case sge:
        return "sge";
    case slt:
        return "slt";
    case sle:
        return "sle";
    default:
        std::unreachable();
    }
}

} // namespace bjac

namespace std {

template <>
struct formatter<::bjac::ICmpInstruction::Kind> final : public formatter<string_view> {
    using formatter<string_view>::parse;

    template <class FmtContext>
    FmtContext::iterator format(::bjac::ICmpInstruction::Kind kind, FmtContext &ctx) const {
        return formatter<string_view>::format(::bjac::to_string_view(kind), ctx);
    }
};

} // namespace std

#endif // INCLUDE_BJAC_IR_ICMP_INSTRUCTION_HPP
