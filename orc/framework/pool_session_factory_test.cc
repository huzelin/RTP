#define private public
#include "orc/framework/pool_session_factory.h"
#undef private
#include "orc/framework/options.h"

#include "gtest/gtest.h"

namespace orc {

class TestSession : public SessionBase { };

using TestPoolSessionFactory = PoolSessionFactory<TestSession>;

TEST(PoolSessionFactory, Init) {
  YAML::Node config;
  TestPoolSessionFactory factory;

  ASSERT_FALSE(factory.Init(config));

  config[Options::SvrSessionPoolSize] = 10;
  config[Options::SvrWorkerNum] = 16;
  ASSERT_FALSE(factory.Init(config));

  config[Options::SvrSessionPoolSize] = 20;
  ASSERT_TRUE(factory.Init(config));

  ASSERT_EQ(20u, factory.sessions_.size());
  auto session = factory.Acquire();
  ASSERT_EQ(19u, factory.sessions_.size());

  factory.Release(session);
  ASSERT_EQ(19u, factory.sessions_.size());
  ASSERT_EQ(1u, factory.tls_sessions_.size());

  auto s1 = factory.Acquire();
  auto s2 = factory.Acquire();
  ASSERT_EQ(18u, factory.sessions_.size());
  ASSERT_EQ(0u, factory.tls_sessions_.size());

  factory.Release(s1);
  ASSERT_EQ(18u, factory.sessions_.size());
  ASSERT_EQ(1u, factory.tls_sessions_.size());

  factory.Release(s2);
  ASSERT_EQ(19u, factory.sessions_.size());
  ASSERT_EQ(1u, factory.tls_sessions_.size());
}

}  // namespace orc
