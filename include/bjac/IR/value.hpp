#ifndef INCLUDE_BJAC_IR_VALUE_HPP
#define INCLUDE_BJAC_IR_VALUE_HPP

#include <memory>

#include "bjac/IR/type.hpp"

namespace bjac {

class Value {
  public:
    Value(std::unique_ptr<Type> type) : type_{std::move(type)} {}

    virtual ~Value() = default;

    const Type &get_type() const noexcept { return *type_; }
    Type::ID get_type_id() const noexcept { return type_->id(); }

  private:
    std::unique_ptr<Type> type_;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_VALUE_HPP
