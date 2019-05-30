#include <fstream>
#include <memory>

#include "gtest/gtest.h"
#include "orc/workflow/compiler.h"
#include "orc/util/log.h"

namespace orc {
namespace wf {

class CompilerTest : public ::testing::Test {
 public:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

TEST_F(CompilerTest, Compile) {
  std::map<std::string, std::unique_ptr<Graph>> graphs;
  Compiler compiler;
  ASSERT_TRUE(compiler.Compile("testdata/compiler_test.wf", &graphs));

  std::string real_str;
  for (const auto& graph : graphs) {
    real_str += graph.second->DebugString();
    real_str += "\n";
  }

  std::ifstream t("testdata/compiler_test.rst");
  std::string expect_str((std::istreambuf_iterator<char>(t)),
                         std::istreambuf_iterator<char>());

  ASSERT_EQ(expect_str, real_str);
}


}  // namespace wf
}  // namespace orc
