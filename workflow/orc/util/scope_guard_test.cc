#include "orc/util/scope_guard.h"

#include "gtest/gtest.h"

namespace orc {

TEST(ScopeGuardTest, L) {
  int i = 0;
  {
    i++;
    auto guard = MakeScopeGuard([&i](){ --i; });
  }

  ASSERT_EQ(int(0), i);
}

TEST(ScopeGuardTest, AL) {
  int i = 0;
  {
    auto guard = MakeScopeGuard([&i](){ ++i; }, [&i](){ --i; });
    ASSERT_EQ(int(1), i);
  }

  ASSERT_EQ(int(0), i);
}

TEST(ScopeGuardTest, Commit) {
  int i = 0;
  {
    auto guard = MakeScopeGuard([&i](){ ++i; });
    guard.Commit();
  }

  ASSERT_EQ(int(0), i);
}

}  // namespace orc
