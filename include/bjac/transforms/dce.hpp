#ifndef INCLUDE_BJAC_TRANSFORMS_DCE_HPP
#define INCLUDE_BJAC_TRANSFORMS_DCE_HPP

#include "bjac/transforms/pass.hpp"

namespace bjac {

class DCE final : public PassMixin<DCE> {
  public:
    DCE() = default;

    void run(Function &f);
};

} // namespace bjac

#endif // INCLUDE_BJAC_TRANSFORMS_DCE_HPP
