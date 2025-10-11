#ifndef INCLUDE_BJAC_IR_VALUE_HPP
#define INCLUDE_BJAC_IR_VALUE_HPP

#include "bjac/IR/type.hpp"

namespace bjac {

class Value {
  public:
    Value(Type type) : type_{type} {}

    virtual ~Value() = default;

    Type get_type() const noexcept { return type_; }

  protected:
    Type type_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_VALUE_HPP
