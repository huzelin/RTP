#ifndef ORC_UTIL_SCOPE_GUARD_H_
#define ORC_UTIL_SCOPE_GUARD_H_

#include <type_traits>
#include <utility>

namespace orc {

template<typename F>
class ScopeGuard {
 private:
  bool committed_;
  F f_;

 public:
  using self_type = ScopeGuard<F>;

  // Exclude copy ctor.
  template<typename L,
           typename std::enable_if<
             !std::is_same<
               typename std::remove_reference<typename std::remove_cv<L>::type>::type,
               self_type
             >::value,
             int*
           >::type = nullptr
  >
  explicit ScopeGuard(L&& l) : committed_(false), f_(std::forward<L>(l)) {}

  template<typename AL, typename L>
  ScopeGuard(AL&& al, L&& l) : committed_(false), f_(std::forward<L>(l)) {  // NOLINT
    std::forward<AL>(al)();
  }

  ScopeGuard(ScopeGuard&& t) : committed_(t.committed_), f_(std::move(t.f_)) {
    t.committed_ = true;
  }

  ~ScopeGuard() {
    if (!committed_) {
      f_();
    }
  }

  void Commit() { committed_ = true; }
};

template<typename AL, typename L>
ScopeGuard<L> MakeScopeGuard(AL&& al, L&& l) {  // NOLINT
  return ScopeGuard<L>(std::forward<AL>(al), std::forward<L>(l));
}

template<typename L>
ScopeGuard<L> MakeScopeGuard(L&& l) {
  return ScopeGuard<L>(std::forward<L>(l));
}

}  // namespace orc

#endif  // ORC_UTIL_SCOPE_GUARD_H_
