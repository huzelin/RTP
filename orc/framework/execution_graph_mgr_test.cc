#include "gtest/gtest.h"
#include "orc/framework/handler_mgr.h"

#define private public
#include "orc/framework/execution_graph_mgr.h"
#undef private

namespace orc {

class MockHandler : public HandlerBase {
 public:
  bool BaseInit(const YAML::Node& config) override { return true; }
  bool BaseRun(SessionBase* session_base, Context* context) override { return true; }
};
ORC_REGISTER_HANDLER(MockHandler);

class Mock1Handler : public HandlerBase {
 public:
  bool BaseInit(const YAML::Node& config) override { return true; }
  bool BaseRun(SessionBase* session_base, Context* context) override { return true; }
};
ORC_REGISTER_HANDLER(Mock1Handler);


class ExecutionGraphMgrTest : public ::testing::Test {
 public:
  virtual void SetUp() {}
  virtual void TearDown() {}

  static void SetUpTestCase() {
    std::string file = "testdata/execution_graph_mgr_config.yaml";
    config_ = YAML::LoadFile(file);
    HandlerMgr::Instance()->Setup(config_);
  }

  static YAML::Node config_;
};

YAML::Node ExecutionGraphMgrTest::config_;

TEST_F(ExecutionGraphMgrTest, TestExeGraph) {
  ASSERT_TRUE(ExecutionGraphMgr::Instance()->Setup(config_));

  ASSERT_EQ(10u, ExecutionGraphMgr::Instance()->pool_size_);
  ASSERT_EQ(std::string("testdata/workflow.ww"), ExecutionGraphMgr::Instance()->src_file_);
  ASSERT_EQ(1u, ExecutionGraphMgr::Instance()->version());

  auto exe_graph = ExecutionGraphMgr::Instance()->GetExeGraph("mock_wf");
  ASSERT_TRUE(exe_graph != nullptr);

  auto bad_case = ExecutionGraphMgr::Instance()->GetExeGraph("bad_wf");
  ASSERT_TRUE(bad_case == nullptr);

  ASSERT_EQ(9u, ExecutionGraphMgr::Instance()->exe_graphs_["mock_wf"].size());

  ExecutionGraphMgr::Instance()->ReleaseExeGraph(exe_graph);
  ASSERT_EQ(9u, ExecutionGraphMgr::Instance()->exe_graphs_["mock_wf"].size());
  ASSERT_EQ(1u, ExecutionGraphMgr::Instance()->tls_exe_graphs_["mock_wf"].size());

  exe_graph = ExecutionGraphMgr::Instance()->GetExeGraph("mock_wf");
  ASSERT_EQ(9u, ExecutionGraphMgr::Instance()->exe_graphs_["mock_wf"].size());
  ASSERT_EQ(0u, ExecutionGraphMgr::Instance()->tls_exe_graphs_["mock_wf"].size());

  // update version
  ASSERT_TRUE(ExecutionGraphMgr::Instance()->Setup(config_));
  ASSERT_EQ(10u, ExecutionGraphMgr::Instance()->exe_graphs_["mock_wf"].size());

  ExecutionGraphMgr::Instance()->ReleaseExeGraph(exe_graph);

  ASSERT_EQ(10u, ExecutionGraphMgr::Instance()->exe_graphs_["mock_wf"].size());
}

}  // namespace orc
