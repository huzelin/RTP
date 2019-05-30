#ifndef ORC_UTIL_CLOSURE_H_
#define ORC_UTIL_CLOSURE_H_

namespace orc {

class Closure {
 public:
  Closure() = default;
  virtual ~Closure() = default;

  virtual void Done() { delete this; }
};

}  // namespace orc

#endif  // ORC_UTIL_CLOSURE_H_
