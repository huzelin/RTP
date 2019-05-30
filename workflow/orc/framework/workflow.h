#ifndef ORC_FRAMEWORK_WORKFLOW_H_
#define ORC_FRAMEWORK_WORKFLOW_H_

#include <memory>

#include "orc/util/macros.h"
#include "orc/util/thread_pool.h"

#include "orc/framework/context.h"
#include "orc/framework/session_factory.h"
#include "orc/framework/workflow_router.h"
#include "orc/framework/service_closure.h"

#include "yaml-cpp/yaml.h"

namespace orc {

class Workflow {
 public:
  Workflow() = default;
  ~Workflow() = default;

  bool Setup(const YAML::Node& config);
  void Stop();

  void Run(ServiceClosure* closure);
  void Run(Context* ctx);

  ThreadPool* thread_pool() const { return thread_pool_.get(); }

 private:
  bool SetupHandlerMgr(const YAML::Node& config);
  bool SetupExecutionGraphMgr(const YAML::Node& config);
  bool InitSessionFactory(const YAML::Node& config);
  bool InitWorkflowRouter(const YAML::Node& config);
  bool InitThreadPool(const YAML::Node& config);

  // Called in Worker
  Context* SetupCtx(ServiceClosure* closure);
  void ResetCtx(Context* ctx);

  void RunInner(ServiceClosure* closure);
  void RunInner(Context* ctx);

 private:
  std::unique_ptr<ThreadPool> thread_pool_;
  SessionFactory* session_factory_;
  WorkflowRouter* workflow_router_;

  ORC_DISALLOW_COPY_AND_ASSIGN(Workflow);
};

}  // namespace orc

#endif  // ORC_FRAMEWORK_WORKFLOW_H_
