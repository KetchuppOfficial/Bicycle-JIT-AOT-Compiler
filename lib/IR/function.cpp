#include <cassert>
#include <ostream>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

#include "bjac/IR/argument_instruction.hpp"
#include "bjac/IR/binary_operator.hpp"
#include "bjac/IR/branch_instruction.hpp"
#include "bjac/IR/call_instruction.hpp"
#include "bjac/IR/constant_instruction.hpp"
#include "bjac/IR/icmp_instruction.hpp"
#include "bjac/IR/ret_instruction.hpp"

#include "bjac/IR/function.hpp"

#include "bjac/graphs/dfs.hpp"

namespace bjac {

void Function::print(std::ostream &os) const {
    using namespace std::string_view_literals;
    auto args =
        arguments_ | std::views::transform([](Type type) static { return to_string_view(type); });
    std::println(os, "{} {}({:s})", return_type_, name_, std::views::join_with(args, ", "sv));
    for (auto &bb : *this) {
        bb.print(os);
    }
}

std::ostream &operator<<(std::ostream &os, const Function &f) {
    f.print(os);
    return os;
}

namespace {

class InlineHelper final {
  public:
    explicit InlineHelper(CallInstruction &call, Function::iterator bb_after_call_it)
        : call_{call}, callee_{call.callee()}, caller_{call.caller()},
          bb_after_call_{*bb_after_call_it},
          ret_val_{try_create_ret_phi(callee_, *bb_after_call_it)},
          callee_bb_to_caller_bb_(
              std::from_range,
              callee_ | std::views::transform([&caller = caller_,
                                               it = bb_after_call_it](BasicBlock &callee_bb) {
                  return std::pair<BasicBlock *const, BasicBlock *>{&callee_bb,
                                                                    &*caller.emplace(it)};
              }),
              callee_.size()),
          callee_instr_to_caller_instr_(callee_.size() * 10) {
        call.get_parent().emplace_back<BranchInstruction>(get_caller_bb(callee_.front()));
    }

    Instruction *ret_value() const { return ret_val_; }

    void clone_instructions() {
        for (auto *callee_bb :
             DFS<MutFunctionGraphTraits>{callee_}.post_order() | std::views::reverse) {
            auto &caller_bb = *callee_bb_to_caller_bb_.find(callee_bb)->second;
            for (auto &callee_instr : *callee_bb) {
                if (auto *caller_instr = clone_instr(caller_bb, callee_instr)) {
                    callee_instr_to_caller_instr_.emplace(&callee_instr, caller_instr);
                }
            }
        }
    }

    void patch_phi_instructions() const {
        for (auto [callee_phi, caller_ptr] : callee_caller_phis_) {
            for (auto [callee_bb, callee_instr] : callee_phi->get_paths()) {
                caller_ptr->add_path(get_caller_bb(*callee_bb), get_caller_instr(callee_instr));
            }
        }
    }

  private:
    Instruction *clone_instr(BasicBlock &caller_bb, Instruction &callee_instr) {
        if (callee_instr.is_binary_op()) {
            return &do_clone(caller_bb, static_cast<BinaryOperator &>(callee_instr));
        }

        using enum Instruction::Opcode;
        switch (callee_instr.get_opcode()) {
        case kArg: { // every use of param shall be replaced by the use of arg
            auto &param = static_cast<ArgumentInstruction &>(callee_instr);
            auto *arg = call_.arguments()[param.get_position()];
            param_to_arg_.emplace(&param, arg);
            return nullptr;
        }
        case kRet: { // all returned values shall later be joined with a PHI instruction
            if (auto &ret = static_cast<ReturnInstruction &>(callee_instr);
                ret.get_ret_type() != Type::kVoid) {
                if (callee_.rets_count() < 2) {
                    assert(!ret_val_);
                    ret_val_ = std::addressof(get_caller_instr(ret.get_ret_value()));
                } else {
                    assert(ret_val_);
                    assert(ret_val_->get_opcode() == Instruction::Opcode::kPHI);
                    auto *ret_phi = static_cast<PHIInstruction *>(ret_val_);
                    ret_phi->add_path(get_caller_bb(ret.get_parent()),
                                      get_caller_instr(ret.get_ret_value()));
                }
            }
            caller_bb.emplace_back<BranchInstruction>(bb_after_call_);
            return nullptr;
        }
        case kPHI: { // simply insert empty PHI instructions, assign paths on the second pass
            auto &phi = static_cast<PHIInstruction &>(callee_instr);
            auto &caller_phi = caller_bb.emplace_back<PHIInstruction>(phi.get_type());
            callee_caller_phis_.emplace_back(std::addressof(phi), std::addressof(caller_phi));
            return &caller_phi;
        }
        case kBr:
            return &do_clone(caller_bb, static_cast<BranchInstruction &>(callee_instr));
        case kCall:
            return &do_clone(caller_bb, static_cast<CallInstruction &>(callee_instr));
        case kICmp:
            return &do_clone(caller_bb, static_cast<ICmpInstruction &>(callee_instr));
        case kConst:
            return &do_clone(caller_bb, static_cast<ConstInstruction &>(callee_instr));
        default:
            std::unreachable();
        }
    };

    Instruction &do_clone(BasicBlock &caller_bb, ConstInstruction &const_instr) const {
        return caller_bb.emplace_back<ConstInstruction>(const_instr.get_type(),
                                                        const_instr.get_value());
    }

    Instruction &do_clone(BasicBlock &caller_bb, BinaryOperator &bin_op) const {
        auto &lhs = get_caller_instr(bin_op.get_lhs());
        auto &rhs = get_caller_instr(bin_op.get_rhs());
        return caller_bb.emplace_back<BinaryOperator>(bin_op.get_opcode(), lhs, rhs);
    }

    Instruction &do_clone(BasicBlock &caller_bb, ICmpInstruction &icmp) const {
        auto &lhs = get_caller_instr(icmp.get_lhs());
        auto &rhs = get_caller_instr(icmp.get_rhs());
        return caller_bb.emplace_back<ICmpInstruction>(icmp.get_kind(), lhs, rhs);
    }

    Instruction &do_clone(BasicBlock &caller_bb, CallInstruction &call) const {
        return caller_bb.emplace_back<CallInstruction>(
            call.callee(),
            std::vector<Instruction *>(
                std::from_range, call.arguments() | std::views::transform([this](Instruction *arg) {
                                     return std::addressof(get_caller_instr(arg));
                                 })));
    }

    Instruction &do_clone(BasicBlock &caller_bb, BranchInstruction &br) const {
        auto &true_path = get_caller_bb(*br.get_true_path());
        if (auto *cond = br.get_condition()) {
            auto &caller_cond = get_caller_instr(cond);
            auto &false_path = get_caller_bb(*br.get_false_path());
            return caller_bb.emplace_back<BranchInstruction>(caller_cond, true_path, false_path);
        } else {
            return caller_bb.emplace_back<BranchInstruction>(true_path);
        }
    }

    BasicBlock &get_caller_bb(BasicBlock &callee_bb) const {
        auto it = callee_bb_to_caller_bb_.find(std::addressof(callee_bb));
        assert(it != callee_bb_to_caller_bb_.end());
        assert(it->second);
        return *it->second;
    }

    Instruction &get_caller_instr(Instruction *callee_instr) const {
        if (auto it = param_to_arg_.find(callee_instr); it != param_to_arg_.end()) {
            assert(it->second);
            return *it->second;
        }

        auto it = callee_instr_to_caller_instr_.find(callee_instr);
        assert(it != callee_instr_to_caller_instr_.end());
        assert(it->second);
        return *it->second;
    }

    static PHIInstruction *try_create_ret_phi(const Function &callee, BasicBlock &bb_after_call) {
        const auto ret_type = callee.return_type();
        if (ret_type == Type::kVoid || callee.rets_count() < 2) {
            return nullptr;
        }
        return std::addressof(bb_after_call.emplace_front<PHIInstruction>(ret_type));
    }

    CallInstruction &call_;
    Function &callee_;
    Function &caller_;
    BasicBlock &bb_after_call_;
    Instruction *ret_val_;
    std::unordered_map<BasicBlock *, BasicBlock *> callee_bb_to_caller_bb_;

    std::unordered_map<Instruction *, Instruction *> callee_instr_to_caller_instr_;
    std::unordered_map<Instruction *, Instruction *> param_to_arg_;

    std::vector<std::pair<PHIInstruction *, PHIInstruction *>> callee_caller_phis_;
};

} // unnamed namespace

void Function::inline_at(CallInstruction &call) {
    if (auto &callee = call.callee(); callee.is_recursive()) {
        throw std::invalid_argument{
            std::format("trying to inline recursive function '{}'", callee.name())};
    }

    auto bb_after_call_it = call.caller().split_bb_at(call);

    // Initialize InlineHelper:
    //   - append the same number of basic blocks to the caller as there are in callee
    //   - create a branch after the call instruction to the first one of new basic blocks
    //   - insert PHI instruction in *bb_after_call_it, if the callee returns non-void type
    InlineHelper helper{call, bb_after_call_it};

    helper.clone_instructions();
    helper.patch_phi_instructions();

    auto call_it = BasicBlock::get_iterator(call);
    if (auto *ret_value = helper.ret_value()) {
        call.get_parent().replace_instruction(call_it, *ret_value);
    } else {
        call.get_parent().erase(call_it);
    }
}

/*
 * bb:
 *   instr_0              bb_0:           bb_1:
 *   ...                    instr_0         instr_(n+1)
 *   instr_(n-1)     ───>     ...           ...
 *   INSTR                  instr_(n-1)     instr_m
 *   instr_(n+1)            INSTR
 *   ...
 *   instr_m
 */
auto Function::split_bb_at(Instruction &instr) -> iterator {
    auto bb_it = Function::get_iterator(instr.get_parent());
    auto pos = std::next(bb_it);
    auto instr_it = BasicBlock::get_iterator(instr);
    auto after_instr_it = std::next(instr_it);

    auto &bb_0 = *bb_it;
    auto bb_1_it = emplace(pos);
    auto &bb_1 = *bb_1_it;

    bb_1.splice(bb_1.end(), bb_0, after_instr_it, bb_0.end());

    return bb_1_it;
}

} // namespace bjac
