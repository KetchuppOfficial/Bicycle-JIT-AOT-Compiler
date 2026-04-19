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

    void set_lhs(Instruction &lhs) {
        if (!lhs_->get_type().is_equal(lhs.get_type())) {
            throw OperandsTypeMismatch{opcode_, lhs_->get_type(), lhs.get_type()};
        }
        lhs_->remove_user(this);
        lhs_ = std::addressof(lhs);
        lhs_->add_user(this);
    }

    template <typename Self>
    auto *get_rhs(this Self &&self) noexcept {
        return std::addressof(std::forward_like<Self>(*self.rhs_));
    }

    void set_rhs(Instruction &rhs) {
        if (!rhs_->get_type().is_equal(rhs.get_type())) {
            throw OperandsTypeMismatch{opcode_, rhs_->get_type(), rhs.get_type()};
        }
        rhs_->remove_user(this);
        rhs_ = std::addressof(rhs);
        rhs_->add_user(this);
    }

    std::string to_string() const override;

    std::vector<Instruction *> inputs() override { return {lhs_, rhs_}; }
    std::vector<const Instruction *> inputs() const override { return {lhs_, rhs_}; }

  private:
    friend class BasicBlock;

    ICmpInstruction(BasicBlock &parent, Kind kind, Instruction &lhs, Instruction &rhs)
        : Instruction(parent, Opcode::kICmp, std::make_unique<IntegralType>(Type::ID::kI1)),
          kind_{kind}, lhs_{std::addressof(lhs)}, rhs_{std::addressof(rhs)} {
        const auto &lhs_type = lhs.get_type();
        const auto &rhs_type = rhs.get_type();
        if (!lhs_type.is_equal(rhs_type)) {
            throw OperandsTypeMismatch{opcode_, lhs_type, rhs_type};
        }

        lhs.add_user(this);
        rhs.add_user(this);
    }

    void remove_as_user() override {
        lhs_->remove_user(this);
        rhs_->remove_user(this);
    }

    Kind kind_;
    Instruction *lhs_;
    Instruction *rhs_;
};

inline std::string_view to_string_view(ICmpInstruction::Kind kind) {
    using namespace std::string_view_literals;
    using enum ICmpInstruction::Kind;
    switch (kind) {
    case eq:
        return "eq"sv;
    case ne:
        return "ne"sv;
    case ugt:
        return "ugt"sv;
    case uge:
        return "uge"sv;
    case ult:
        return "ult"sv;
    case ule:
        return "ule"sv;
    case sgt:
        return "sgt"sv;
    case sge:
        return "sge"sv;
    case slt:
        return "slt"sv;
    case sle:
        return "sle"sv;
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
