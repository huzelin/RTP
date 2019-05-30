#ifndef ORC_FRAMEWORK_CONTEXT_H_
#define ORC_FRAMEWORK_CONTEXT_H_

#include <sys/time.h>

#include <atomic>
#include <vector>
#include <functional>

#include "orc/util/closure.h"

namespace orc {

// Forward declare
class SessionBase;
class Workflow;
class ExecutionGraph;
class ExecutionNode;

class Context {
 public:
  explicit Context(SessionBase* session);
  ~Context() = default;

  void Setup(Workflow* workflow, ExecutionGraph* graph);
  void Reset();

  void Callback();

  void set_schedule_hint(int32_t hint) { schedule_hint_ = hint; }
  int32_t schedule_hint() { return schedule_hint_; }

  bool IsAsync();
  void SetAsync(bool async);
  bool IsDone();

  void Step();

  Workflow* workflow() const { return workflow_; }

  ExecutionNode* node() const { return node_; }
  void set_node(ExecutionNode* node) { node_ = node; }

  bool next_edge() const { return next_edge_; }
  void set_next_edge(bool next_edge) { next_edge_ = next_edge; }

  enum class AsyncState { Before, After, Finish };

  void set_async_state(AsyncState state) { async_state_ = state; }
  AsyncState async_state() const { return async_state_; }

  void set_async_closure(Closure* closure) { async_closure_ = closure; }
  Closure* async_closure() const { return async_closure_; }

  SessionBase* session() const { return session_; }
  ExecutionGraph* graph() const { return graph_; }

  const std::vector<ExecutionNode*>& walked_path() const { return walked_path_; }

 private:
  void StepReset();
  void NextStep();

 private:
  SessionBase* session_;

  ExecutionNode* node_;
  bool next_edge_;
  std::vector<ExecutionNode*> walked_path_;
  struct timeval start_time_;

  bool is_async_;
  std::atomic<AsyncState> async_state_;
  std::atomic<int32_t> async_counter_;
  Closure* async_closure_;

  int32_t schedule_hint_;
  ExecutionGraph* graph_;
  Workflow* workflow_;

  int32_t lives_;
};

}  // namespace orc

#endif  // ORC_FRAMEWORK_CONTEXT_H_
