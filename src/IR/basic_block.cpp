#include "bjac/IR/basic_block.hpp"
#include "bjac/IR/function.hpp"

namespace bjac {

BasicBlock::BasicBlock(Function &parent) noexcept
    : parent_{std::addressof(parent)}, id_{parent.get_next_bb_id()} {}

} // namespace bjac
