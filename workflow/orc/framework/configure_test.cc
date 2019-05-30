#include "gtest/gtest.h"
#include "orc/framework/configure.h"
#include "orc/util/utils.h"

namespace orc {

class ConfigureTest : public ::testing::Test {
 public:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

TEST_F(ConfigureTest, Configure) {
  ASSERT_TRUE(InitOrcConfig("testdata/orc.yaml"));
  YAML::Node& config = OrcConfig();

  std::string str = YAML::Dump(config);

  bool daemon;
  ASSERT_TRUE(GetOrcConfig(Options::OrcDaemon, &daemon));
  ASSERT_FALSE(daemon);
  ASSERT_TRUE(config["servers"]);
  ASSERT_TRUE(config["servers"].IsSequence());
  int32_t io_thread_num;
  ASSERT_TRUE(GetOrcConfig(config["servers"][0], Options::SvrIoThreadNum, &io_thread_num));
  ASSERT_EQ(1, io_thread_num);
}

}  // namespace orc
