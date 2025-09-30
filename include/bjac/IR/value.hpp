#ifndef INCLUDE_BJAC_IR_VALUE_HPP
#define INCLUDE_BJAC_IR_VALUE_HPP

namespace bjac {

class Value {
  public:
    template <typename T>
    static const void *to_void_ptr(const T *ptr) {
        return static_cast<const void *>(static_cast<const Value *>(ptr));
    }

    virtual ~Value() = default;
};

} // namespace bjac

#endif // INCLUDE_BJAC_IR_VALUE_HPP
