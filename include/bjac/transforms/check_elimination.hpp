#ifndef INCLUDE_BJAC_TRANSFROMS_CHECK_ELIMINATION_HPP
#define INCLUDE_BJAC_TRANSFROMS_CHECK_ELIMINATION_HPP

#include "bjac/transforms/pass.hpp"

namespace bjac {

class CheckEliminationPass final : public PassMixin<CheckEliminationPass> {
  public:
    CheckEliminationPass() = default;

    void run(Function &f);
};

} // namespace bjac

#endif // INCLUDE_BJAC_TRANSFROMS_CHECK_ELIMINATION_HPP
