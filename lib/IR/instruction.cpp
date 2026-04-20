#include <format>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>

#include "bjac/IR/basic_block.hpp"
#include "bjac/IR/function.hpp"

#include "bjac/IR/argument_instruction.hpp"
#include "bjac/IR/binary_operator.hpp"
#include "bjac/IR/bounds_check.hpp"
#include "bjac/IR/branch_instruction.hpp"
#include "bjac/IR/call_instruction.hpp"
#include "bjac/IR/constant_instruction.hpp"
#include "bjac/IR/icmp_instruction.hpp"
#include "bjac/IR/load_instruction.hpp"
#include "bjac/IR/null_check.hpp"
#include "bjac/IR/phi_instruction.hpp"
#include "bjac/IR/ret_instruction.hpp"

namespace bjac {

Instruction::Instruction(BasicBlock &parent, Opcode opcode, std::unique_ptr<Type> type)
    : Value{std::move(type)}, opcode_{opcode}, parent_{std::addressof(parent)},
      id_{parent.get_next_instr_id()} {}

void Instruction::replace_with(Instruction &other) {
    for (auto it = users_.begin(), ite = users_.end(); it != ite;) {
        auto *user = *it;
        if (user->is_binary_op()) {
            if (auto *bin_op = static_cast<BinaryOperator *>(user); bin_op->get_lhs() == this) {
                if (bin_op->get_rhs() == this) {
                    it = std::next(it, 2);
                    bin_op->set_lhs(other);
                    bin_op->set_rhs(other);
                } else {
                    ++it;
                    bin_op->set_lhs(other);
                }
            } else if (bin_op->get_rhs() == this) {
                ++it;
                bin_op->set_rhs(other);
            } else {
                ++it;
            }
        } else {
            switch (user->get_opcode()) {
            case Opcode::kICmp:
                if (auto *icmp = static_cast<ICmpInstruction *>(user); icmp->get_lhs() == this) {
                    if (icmp->get_rhs() == this) {
                        it = std::next(it, 2);
                        icmp->set_lhs(other);
                        icmp->set_rhs(other);
                    } else {
                        ++it;
                        icmp->set_lhs(other);
                    }
                } else if (icmp->get_rhs() == this) {
                    ++it;
                    icmp->set_rhs(other);
                } else {
                    ++it;
                }
                break;
            case Opcode::kRet:
                ++it;
                static_cast<ReturnInstruction *>(user)->set_ret_value(other);
                break;
            case Opcode::kBr:
                ++it;
                static_cast<BranchInstruction *>(user)->set_condition(other);
                break;
            case Opcode::kPHI:
                ++it;
                static_cast<PHIInstruction *>(user)->replace_value(*this, other);
                break;
            default:
                ++it;
                break;
            }
        }
    }
}

namespace {

std::unique_ptr<Type> get_arg_type(const Function &f, unsigned pos) {
    if (const auto args = f.arguments(); pos < args.size()) {
        return args[pos]->clone();
    }
    throw ArgOutOfRange{"function parameter index is out of range"};
}

std::string ssa_value_to_string(const Instruction &instr) {
    return std::format("%{}.{}", instr.get_parent().get_id(), instr.get_id());
}

std::string users_to_string(const Instruction &instr) {
    std::ranges::bidirectional_range auto users = instr.get_users();
    if (users.empty()) {
        return {};
    }

    using namespace std::string_view_literals;
    auto values = users | std::views::transform(
                              [](const auto *user) static { return ssa_value_to_string(*user); });
    return std::format(" ; used by: {:s}", std::views::join_with(values, ", "sv));
}

} // unnamed namespace

ArgumentInstruction::ArgumentInstruction(BasicBlock &parent, unsigned pos)
    : Instruction(parent, Opcode::kArg, get_arg_type(parent.get_parent(), pos)), pos_{pos} {}

std::string ArgumentInstruction::to_string() const {
    return std::format("{} = {} {} [{}]{}", ssa_value_to_string(*this), get_type().to_string(),
                       Opcode::kArg, pos_, users_to_string(*this));
}

std::string BinaryOperator::to_string() const {
    return std::format("{} = {} {} {}, {}{}", ssa_value_to_string(*this), get_type().to_string(),
                       opcode_, ssa_value_to_string(*lhs_), ssa_value_to_string(*rhs_),
                       users_to_string(*this));
}

std::string BranchInstruction::to_string() const {
    if (is_conditional()) {
        return std::format("{} {} {} {}, label %bb{}, label %bb{}{}", ssa_value_to_string(*this),
                           Opcode::kBr, condition_->get_type().to_string(),
                           ssa_value_to_string(*condition_), get_true_path()->get_id(),
                           get_false_path()->get_id(), users_to_string(*this));
    } else {
        return std::format("{} {} label %bb{}{}", ssa_value_to_string(*this), Opcode::kBr,
                           get_true_path()->get_id(), users_to_string(*this));
    }
}

std::string ConstInstruction::to_string() const {
    return std::format("{} = {} {} {}{}", ssa_value_to_string(*this), get_type().to_string(),
                       Opcode::kConst, value_, users_to_string(*this));
}

std::string ICmpInstruction::to_string() const {
    return std::format("{} = {} {} {} {}, {}{}", ssa_value_to_string(*this), Opcode::kICmp, kind_,
                       lhs_->get_type().to_string(), ssa_value_to_string(*lhs_),
                       ssa_value_to_string(*rhs_), users_to_string(*this));
}

std::string PHIInstruction::to_string() const {
    if (records_.empty()) {
        return std::format("{} = {} {}{}", ssa_value_to_string(*this), Opcode::kPHI,
                           get_type().to_string(), users_to_string(*this));
    } else {
        auto phi_strings = records_ | std::views::transform([](const auto &r) static {
                               return std::format("[{}, %bb{}]", ssa_value_to_string(*r.second),
                                                  r.first->get_id());
                           });
        using namespace std::string_view_literals;
        return std::format("{} = {} {} {:s}{}", ssa_value_to_string(*this), Opcode::kPHI,
                           get_type().to_string(), std::views::join_with(phi_strings, ", "sv),
                           users_to_string(*this));
    }
}

ReturnInstruction::ReturnInstruction(BasicBlock &parent)
    : Instruction(parent, Opcode::kRet, std::make_unique<VoidType>()), ret_val_{nullptr} {
    if (auto ret_type_id = parent.get_parent().return_type_id(); ret_type_id != Type::ID::kVoid) {
        throw std::invalid_argument{std::format("trying to create {} {} in a function returning {}",
                                                Opcode::kRet, Type::ID::kVoid, ret_type_id)};
    }
}

ReturnInstruction::ReturnInstruction(BasicBlock &parent, Instruction &ret_val)
    : Instruction(parent, Opcode::kRet, std::make_unique<VoidType>()),
      ret_val_{std::addressof(ret_val)} {
    auto &callee = parent.get_parent();
    auto &owner = ret_val.get_parent().get_parent();
    if (std::addressof(owner) != std::addressof(callee)) {
        throw std::invalid_argument{
            std::format("trying to return SSA value owned by function '{}' from function '{}'",
                        owner.name(), callee.name())};
    }

    const auto &ret_type = parent.get_parent().return_type();
    if (!ret_type.is_equal(ret_val.get_type())) {
        throw std::invalid_argument{std::format("trying to create {} {} in a function returning {}",
                                                Opcode::kRet, ret_val.get_type().to_string(),
                                                ret_type.to_string())};
    }

    ret_val.add_user(this);
}

std::string ReturnInstruction::to_string() const {
    if (ret_val_) {
        return std::format("{} {} {} {}", ssa_value_to_string(*this), Opcode::kRet,
                           ret_val_->get_type().to_string(), ssa_value_to_string(*ret_val_));
    }
    return std::format("{} {} {}", ssa_value_to_string(*this), Opcode::kRet, Type::ID::kVoid);
}

CallInstruction::CallInstruction(BasicBlock &parent, Function &callee,
                                 std::vector<Instruction *> args)
    : Instruction(parent, Opcode::kCall, callee.return_type().clone()),
      callee_{std::addressof(callee)}, args_(std::move(args)) {
    auto parameters =
        callee.arguments() |
        std::views::transform([](const auto &u_ptr) static -> const Type & { return *u_ptr; });
    auto arguments = args_ | std::views::transform([](const auto *arg) static -> const Type & {
                         return arg->get_type();
                     });
    if (!std::ranges::equal(parameters, arguments,
                            [](const auto &lhs, const auto &rhs) { return lhs.is_equal(rhs); })) {
        throw std::invalid_argument{std::format(
            "types of call arguments mismatch with parameters of function '{}'", callee.name())};
    }
}

std::string CallInstruction::to_string() const {
    std::ranges::view auto arg_string = args_ | std::views::transform([](const auto *arg) static {
                                            return ssa_value_to_string(*arg);
                                        });
    using namespace std::string_view_literals;
    return std::format("{} = {} {}({:s})", ssa_value_to_string(*this), Opcode::kCall,
                       callee_->name(), std::views::join_with(arg_string, ", "sv));
}

Function &CallInstruction::caller() noexcept { return get_parent().get_parent(); }
const Function &CallInstruction::caller() const noexcept { return get_parent().get_parent(); }

NullCheckInstruction::NullCheckInstruction(BasicBlock &parent, Instruction &input)
    : Instruction{parent, Opcode::kNullCheck, std::make_unique<VoidType>()},
      input_{std::addressof(input)} {
    if (input.get_type_id() != Type::ID::kPointer) {
        throw std::invalid_argument{
            std::format("'{}' does not have pointer type and cannot be used as the "
                        "input of a {} instruction",
                        input.to_string(), Opcode::kNullCheck)};
    }
}

std::string NullCheckInstruction::to_string() const {
    return std::format("{} {} {} {}", ssa_value_to_string(*this), Opcode::kNullCheck,
                       Type::ID::kPointer, ssa_value_to_string(*input_));
}

BoundsCheckInstruction::BoundsCheckInstruction(BasicBlock &parent, Instruction &array,
                                               Instruction &index)
    : Instruction{parent, Opcode::kBoundsCheck, std::make_unique<VoidType>()},
      array_{std::addressof(array)}, index_{std::addressof(index)} {
    if (array.get_type_id() != Type::ID::kArray) {
        throw std::invalid_argument{std::format(
            "'{}' does not have array type and cannot be used as an input of a {} instruction",
            array.to_string(), Opcode::kBoundsCheck)};
    }
    if (index.get_type_id() != Type::ID::kI64) {
        throw std::invalid_argument{std::format(
            "'{}' does not have {} type and cannot be used as an input of a {} instruction",
            index.to_string(), Type::ID::kI64, Opcode::kBoundsCheck)};
    }
}

std::string BoundsCheckInstruction::to_string() const {
    return std::format("{} {} {} {}, {} {}", ssa_value_to_string(*this), Opcode::kBoundsCheck,
                       array_->get_type().to_string(), ssa_value_to_string(*array_), Type::ID::kI64,
                       ssa_value_to_string(*index_));
}

LoadInstruction::LoadInstruction(BasicBlock &parent, std::unique_ptr<Type> type, Instruction &addr)
    : Instruction(parent, Opcode::kLoad, std::move(type)), addr_{std::addressof(addr)} {
    using enum Type::ID;

    switch (const auto type_id = get_type_id()) {
    case kNone:
    case kVoid:
    case kArray:
        throw std::invalid_argument{std::format("'{}' cannot be loaded", type_id)};
    default:
        break;
    }

    if (addr.get_type_id() != kPointer) {
        throw std::invalid_argument{std::format(
            "address of load shall be represented with a value of pointer type, not '{}'",
            addr.to_string())};
    }
}

std::string LoadInstruction::to_string() const {
    return std::format("{} = {} {}, {} {}{}", ssa_value_to_string(*this), Opcode::kLoad,
                       get_type().to_string(), Type::ID::kPointer, ssa_value_to_string(*addr_),
                       users_to_string(*this));
}

} // namespace bjac
