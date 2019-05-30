#include "orc/framework/context.h"

#include <string.h>

#include "orc/framework/session_base.h"
#include "orc/framework/execution_graph.h"
#include "orc/framework/workflow.h"
#include "orc/util/log.h"
#include "orc/util/utils.h"

#include "common/monitor/monitor_status_impl.h"

namespace orc {

Context::Context(SessionBase* session)
    : session_(session),
      node_(ExecutionNode::EndNode()),
      next_edge_(false), is_async_(false),
      async_state_(AsyncState::Before),
      async_counter_(0),
      async_closure_(nullptr),
      schedule_hint_(-1),
      graph_(nullptr),
      workflow_(nullptr) {}

void Context::Setup(Workflow* workflow, ExecutionGraph* graph) {
  workflow_ = workflow;
  graph_ = graph;
  node_ = graph->entry();
  walked_path_.clear();
  StepReset();
  gettimeofday(&start_time_, nullptr);
  lives_ = 1;
}

void Context::Reset() {
  workflow_ = nullptr;
  graph_ = nullptr;
  node_ = nullptr;
  walked_path_.clear();
  schedule_hint_ = -1;
  StepReset();
  --lives_;
}

void Context::Callback() {
  set_async_state(AsyncState::After);
  workflow_->Run(this);
}

bool Context::IsAsync() {
  return is_async_ && ((async_counter_--) > 0);
}

void Context::SetAsync(bool async) {
  if (async) {
    async_counter_ = 1;
    async_state_ = AsyncState::Before;
  }
  is_async_ = async;
}

void Context::Step() {
  node()->Run(session_, this);

  if ((!is_async_) || (async_state() == AsyncState::Finish)) {
    NextStep();
  }
}

void Context::NextStep() {
  walked_path_.emplace_back(node());

  struct timeval now;
  gettimeofday(&now, NULL);

  int64_t escape = (now.tv_sec - start_time_.tv_sec) * 1000000;
  escape += now.tv_usec - start_time_.tv_usec;

  MONITOR_STATUS_NORMAL_TIMER_BY("handler", node()->name().c_str(), escape, 1);

  memcpy(&start_time_, &now, sizeof(now));

  if (next_edge()) {
    set_node(node()->true_next());
  } else {
    set_node(node()->false_next());
  }

  StepReset();
}

void Context::StepReset() {
  is_async_ = false;
  async_state_ = AsyncState::Before;
  async_counter_ = 0;
  if (async_closure_ != nullptr) {
    async_closure_->Done();
    async_closure_ = nullptr;
  }
}

bool Context::IsDone() {
  return node_ == ExecutionNode::EndNode();
}

}  // namespace orc
