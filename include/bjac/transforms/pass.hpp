#ifndef INCLUDE_BJAC_TRANSFORMS_PASS
#define INCLUDE_BJAC_TRANSFORMS_PASS

#include "bjac/IR/function.hpp"

namespace bjac {

template <typename Pass>
class PassMixin {
  public:
    void run(Function &f) { static_cast<Pass *>(this)->run(f); }
};

} // namespace bjac

#endif // INCLUDE_BJAC_TRANSFORMS_PASS
