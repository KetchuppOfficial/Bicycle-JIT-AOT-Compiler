#ifndef INCLUDE_BJAC_ANALYSIS_REG_ALLOC_HPP
#define INCLUDE_BJAC_ANALYSIS_REG_ALLOC_HPP

#include <cstddef>
#include <format>
#include <memory>
#include <unordered_map>

namespace bjac {

class Function;
class Instruction;

class RegAlloc final {
  public:
    struct Storage {
        enum Kind : bool { kRegister, kStackSlot };

        Kind kind;
        std::size_t index;

        constexpr bool operator==(const Storage &) const = default;
    };

  private:
    using Container = std::unordered_map<const Instruction *, Storage>;

  public:
    using const_iterator = Container::const_iterator;
    using iterator = const_iterator;

    explicit RegAlloc(const Function &func, std::size_t free_regs_count);

    const Storage &at(const Instruction &instr) const {
        return instr_to_reg_.at(std::addressof(instr));
    }

    const_iterator begin() const noexcept { return instr_to_reg_.begin(); }
    const_iterator cbegin() const noexcept { return instr_to_reg_.cbegin(); }

    const_iterator end() const noexcept { return instr_to_reg_.end(); }
    const_iterator cend() const noexcept { return instr_to_reg_.cend(); }

  private:
    Container instr_to_reg_;
};

} // namespace bjac

namespace std {

template <>
struct formatter<::bjac::RegAlloc::Storage> : formatter<string> {
    using formatter<string>::parse;

    template <typename FmContext>
    typename FmContext::iterator format(const ::bjac::RegAlloc::Storage &storage,
                                        FmContext &ctx) const {
        using enum ::bjac::RegAlloc::Storage::Kind;
        switch (storage.kind) {
        case kRegister:
            return format_to(ctx.out(), "register[{}]", storage.index);
        case kStackSlot:
            return format_to(ctx.out(), "stack[{}]", storage.index);
        }
    }
};

} // namespace std

#endif // INCLUDE_BJAC_ANALYSIS_REG_ALLOC_HPP
