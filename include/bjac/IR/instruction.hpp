#ifndef INCLUDE_BJAC_IR_INSTRUCTION_HPP
#define INCLUDE_BJAC_IR_INSTRUCTION_HPP

#include <cstdint>
#include <format>
#include <map>
#include <memory>
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

    virtual std::string to_string() const = 0;

  private:
    friend class BasicBlock;

    Instruction(BasicBlock &parent, Opcode opcode) noexcept
        : parent_{std::addressof(parent)}, opcode_{opcode} {}

    template <Opcode kBegin, Opcode kEnd>
    static constexpr bool is_in_category(Opcode opcode) noexcept {
        auto opc = std::to_underlying(opcode);
        return std::to_underlying(kBegin) <= opc && opc < std::to_underlying(kEnd);
    }

  protected:
    BasicBlock *parent_ = nullptr;
    Opcode opcode_;
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

class BinaryOperator : public Instruction {
  public:
    static std::unique_ptr<BinaryOperator> create(Opcode opcode, Value &lhs, Value &rhs) {
        return std::unique_ptr<BinaryOperator>{new BinaryOperator{opcode, lhs, rhs}};
    }

    ~BinaryOperator() override = default;

    Value *get_lhs() noexcept { return lhs_; }
    const Value *get_lhs() const noexcept { return lhs_; }

    Value *get_rhs() noexcept { return rhs_; }
    const Value *get_rhs() const noexcept { return rhs_; }

    std::string to_string() const override {
        return std::format("%{} = {} %{} %{}", Value::to_void_ptr(this), to_string_view(opcode_),
                           Value::to_void_ptr(lhs_), Value::to_void_ptr(rhs_));
    }

  protected:
    BinaryOperator(Opcode opcode, Value &lhs, Value &rhs)
        : Instruction(opcode), lhs_{&lhs}, rhs_{&rhs} {
        if (opcode < Opcode::kBinaryBegin || opcode >= Opcode::kBinaryEnd) {
            throw std::invalid_argument{"invalid opcode for a binary operator"};
        }
    }

  private:
    Value *lhs_;
    Value *rhs_;
};

class ReturnInstruction : public Instruction {
  public:
    static std::unique_ptr<ReturnInstruction> create() {
        return std::unique_ptr<ReturnInstruction>{new ReturnInstruction{}};
    }

    static std::unique_ptr<ReturnInstruction> create(Value &ret_val) {
        return std::unique_ptr<ReturnInstruction>{new ReturnInstruction{ret_val}};
    }

    ~ReturnInstruction() override = default;

    Value *get_ret_value() noexcept { return ret_val_; }
    const Value *get_ret_value() const noexcept { return ret_val_; }

    std::string to_string() const override {
        if (ret_val_) {
            return std::format("{} %{}", to_string_view(opcode_), Value::to_void_ptr(ret_val_));
        }
        return std::format("{} {}", to_string_view(opcode_), to_string_view(Type::kVoid));
    }

  protected:
    ReturnInstruction() : Instruction(Opcode::kRet), ret_val_{nullptr} {} // ret void
    ReturnInstruction(Value &ret_val) : Instruction(Opcode::kRet), ret_val_{&ret_val} {}

  private:
    Value *ret_val_;
};

class BranchInstruction : public Instruction {
  public:
    static std::unique_ptr<BranchInstruction> create(BasicBlock &true_path) {
        return std::unique_ptr<BranchInstruction>{new BranchInstruction{true_path}};
    }

    static std::unique_ptr<BranchInstruction> create(Value &condition, BasicBlock &true_path,
                                                     BasicBlock &false_path) {
        return std::unique_ptr<BranchInstruction>{
            new BranchInstruction{condition, true_path, false_path}};
    }

    ~BranchInstruction() override = default;

    bool is_conditional() const noexcept { return condition_ != nullptr; }

    Value *get_condition() noexcept { return condition_; }
    const Value *get_condition() const noexcept { return condition_; }

    BasicBlock *get_true_path() noexcept { return true_path_; }
    const BasicBlock *get_true_path() const noexcept { return true_path_; }
    void set_true_path(BasicBlock &bb) noexcept { true_path_ = &bb; }

    BasicBlock *get_false_path() noexcept { return false_path_; }
    const BasicBlock *get_false_path() const noexcept { return false_path_; }
    void set_false_path(BasicBlock &bb) noexcept { false_path_ = &bb; }

    std::string to_string() const override;

  protected:
    BranchInstruction(BasicBlock &true_path)
        : Instruction(Opcode::kBr), condition_{nullptr}, true_path_{&true_path},
          false_path_{nullptr} {}

    BranchInstruction(Value &condition, BasicBlock &true_path, BasicBlock &false_path)
        : Instruction(Opcode::kBr), condition_{&condition}, true_path_{&true_path},
          false_path_{&false_path} {}

  private:
    Value *condition_;
    BasicBlock *true_path_;
    BasicBlock *false_path_;
};

class ICmpInstruction : public Instruction {
  public:
    enum class Kind { eq, ne, ugt, uge, ult, ule, sgt, sge, slt, sle };

    static std::unique_ptr<ICmpInstruction> create(Kind kind) {
        return std::unique_ptr<ICmpInstruction>{new ICmpInstruction{kind}};
    }

    static std::unique_ptr<ICmpInstruction> create(Kind kind, Value &lhs, Value &rhs) {
        return std::unique_ptr<ICmpInstruction>{new ICmpInstruction{kind, lhs, rhs}};
    }

    ~ICmpInstruction() override = default;

    Kind get_kind() const noexcept { return kind_; }

    Value *get_lhs() noexcept { return lhs_; }
    const Value *get_lhs() const noexcept { return lhs_; }
    void set_lhs(Value &lhs) noexcept { lhs_ = &lhs; }

    Value *get_rhs() noexcept { return rhs_; }
    const Value *get_rhs() const noexcept { return rhs_; }
    void set_rhs(Value &rhs) noexcept { rhs_ = &rhs; }

    std::string to_string() const override;

  protected:
    ICmpInstruction(Kind kind)
        : Instruction(Opcode::kICmp), kind_{kind}, lhs_{nullptr}, rhs_{nullptr} {}

    ICmpInstruction(Kind kind, Value &lhs, Value &rhs)
        : Instruction(Opcode::kICmp), kind_{kind}, lhs_{&lhs}, rhs_{&rhs} {}

  private:
    Kind kind_;
    Value *lhs_;
    Value *rhs_;
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
struct formatter<::bjac::ICmpInstruction::Kind> : public formatter<string_view> {
    template <typename ParseConstexpr>
    constexpr ParseConstexpr::iterator parse(ParseConstexpr &ctx) {
        return formatter<string_view>::parse(ctx);
    }

    template <class FmtContext>
    FmtContext::iterator format(::bjac::ICmpInstruction::Kind kind, FmtContext &ctx) const {
        return formatter<string_view>::format(::bjac::to_string_view(kind), ctx);
    }
};

} // namespace std

namespace bjac {

inline std::string ICmpInstruction::to_string() const {
    return std::format("%{} = {} {} {}, {}", Value::to_void_ptr(this), to_string_view(opcode_),
                       kind_, Value::to_void_ptr(lhs_), Value::to_void_ptr(rhs_));
}

class PHIInstruction : public Instruction {
  public:
    static std::unique_ptr<PHIInstruction> create() {
        return std::unique_ptr<PHIInstruction>{new PHIInstruction{}};
    }

    ~PHIInstruction() override = default;

    void add(BasicBlock &bb, Value &value) { records_.emplace(&bb, &value); }

    template <typename Self>
    auto get(this Self &&self, BasicBlock &bb)
        -> std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const Value *,
                              Value *> {
        if (auto it = self.records_.find(&bb); it != self.records_.end()) {
            return it->second;
        }
        return nullptr;
    }

    std::string to_string() const override;

  protected:
    PHIInstruction() : Instruction(Opcode::kPHI) {}

  private:
    std::map<BasicBlock *, Value *> records_;
};

class ArgumentInstruction : public Instruction {
  public:
    static std::unique_ptr<ArgumentInstruction> create(unsigned pos) {
        return std::unique_ptr<ArgumentInstruction>{new ArgumentInstruction{pos}};
    }

    ~ArgumentInstruction() override = default;

    unsigned get_position() const noexcept { return pos_; }

    std::string to_string() const override {
        return std::format("%{} = {} {}", Value::to_void_ptr(this), to_string_view(opcode_), pos_);
    }

  protected:
    ArgumentInstruction(unsigned pos) : Instruction(Opcode::kArg), pos_{pos} {}

  private:
    unsigned pos_;
};

class ConstInstruction : public Instruction {
  public:
    static std::unique_ptr<ConstInstruction> create(Type type, std::uintmax_t value) {
        return std::unique_ptr<ConstInstruction>{new ConstInstruction{type, value}};
    }

    ~ConstInstruction() = default;

    Type get_type() const noexcept { return type_; }
    std::uintmax_t get_value() const noexcept { return value_; }

    std::string to_string() const override {
        return std::format("%{} = {} {} {}", Value::to_void_ptr(this), to_string_view(opcode_),
                           type_, value_);
    }

  protected:
    ConstInstruction(Type type, std::uintmax_t value)
        : Instruction(Opcode::kConst), type_{type}, value_{value} {
        if (type == Type::kVoid) {
            throw std::invalid_argument{"void constant shall not be created"};
        }
    }

  private:
    Type type_;
    std::uintmax_t value_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_INSTRUCTION_HPP
