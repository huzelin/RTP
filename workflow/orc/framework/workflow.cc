#include "orc/framework/workflow.h"

#include "orc/framework/configure.h"
#include "orc/util/utils.h"
#include "orc/framework/session_factory_mgr.h"
#include "orc/framework/workflow_router_mgr.h"
#include "orc/framework/handler_mgr.h"
#include "orc/framework/execution_graph_mgr.h"

#include "common/monitor/monitor_status_impl.h"

namespace orc {

bool Workflow::SetupHandlerMgr(const YAML::Node& config) {
  if (!HandlerMgr::Instance()->Setup(config)) {
    ORC_ERROR("Init HandlerMgr fail.");
    return false;
  }

  ORC_INFO("Init HandlerMgr success.");
  return true;
}

bool Workflow::SetupExecutionGraphMgr(const YAML::Node& config) {
  if (!ExecutionGraphMgr::Instance()->Setup(config)) {
    ORC_ERROR("Init ExecutionGraphMgr fail.");
    return false;
  }

  ORC_INFO("Init ExecutionGraphMgr success.");
  return true;
}

bool Workflow::InitSessionFactory(const YAML::Node& config) {
  std::string factory;
  ORC_CONFIG_OR_FAIL(config, Options::SvrSessionFactory, factory);
  session_factory_ = SessionFactoryMgr::Instance()->GetSessionFactory(factory);
  if (session_factory_ == nullptr) {
    ORC_ERROR("SessionFactory: %s not register.", factory.c_str());
    return false;
  }

  if (!session_factory_->Init(config)) {
    ORC_ERROR("SessionFactory: %s Init fail.", factory.c_str());
    return false;
  }
  ORC_INFO("SessionFactory: %s Init success.", factory.c_str());

  return true;
}

bool Workflow::InitWorkflowRouter(const YAML::Node& config) {
  std::string router;
  ORC_CONFIG_OR_FAIL(config, Options::SvrWorkflowRouter, router);
  workflow_router_ = WorkflowRouterMgr::Instance()->GetWorkflowRouter(router);
  if (workflow_router_ == nullptr) {
    ORC_ERROR("WorkflowRouter: %s not register.", router.c_str());
    return false;
  }

  if (!workflow_router_->Init(config)) {
    ORC_ERROR("WorkflowRouter: %s Init fail.", router.c_str());
    return false;
  }
  ORC_INFO("WorkflowRouter: %s Init success.", router.c_str());

  return true;
}

bool Workflow::InitThreadPool(const YAML::Node& config) {
  ThreadPool::Option option;
  ORC_CONFIG_OR_FAIL(config, Options::SvrWorkerNum, option.worker_num);
  ORC_CONFIG_OR_FAIL(config, Options::SvrWorkerQueueNum, option.queue_num);
  ORC_CONFIG_OR_FAIL(config, Options::SvrWorkerQueueSize, option.queue_size);
  ORC_CONFIG_OR_DEFAULT(config, Options::SvrWorkerPinCpu, option.pin_cpu, false);
  thread_pool_.reset(new ThreadPool(option));
  ORC_INFO("ThreadPool Init success.");
  return true;
}

bool Workflow::Setup(const YAML::Node& config) {
  if (!SetupHandlerMgr(config) ||
      !SetupExecutionGraphMgr(config) ||
      !InitSessionFactory(config) ||
      !InitWorkflowRouter(config) ||
      !InitThreadPool(config)) {
    ORC_ERROR("Workflow Setup fail.");
    return false;
  }

  ORC_INFO("Workflow Setup success.");
  return true;
}

void Workflow::Stop() {
  thread_pool_.reset(nullptr);
}

void Workflow::Run(ServiceClosure* closure) {
  ThreadMeta* thread_meta = GetThreadMeta();
  if (thread_meta->pool != thread_pool_.get()) {
    auto task = thread_pool_->Schedule([this, closure](){ RunInner(closure); });
    if (task) {
      // Schedule fail.
      ORC_WARN("ThreadPool Schedule fail for ServiceClosure.");
      closure->Done();
    }
  } else {
    RunInner(closure);
  }
}

void Workflow::Run(Context* ctx) {
  ThreadMeta* thread_meta = GetThreadMeta();
  if (thread_meta->pool != thread_pool_.get()) {
    auto task = thread_pool_->Schedule([this, ctx](){ RunInner(ctx); });
    if (task) {
      // Schedule fail.
      ORC_WARN("ThreadPool Schedule fail for Context. hint: %d", ctx->schedule_hint());
      ResetCtx(ctx);
    }
  } else {
    RunInner(ctx);
  }
}

Context* Workflow::SetupCtx(ServiceClosure* closure) {
  auto session = session_factory_->Acquire();
  if (session == nullptr) {
    ORC_WARN("Acquire Session fail.");
    closure->set_ret_code(ServiceClosure::RetCode::NoSession);
    closure->set_message(std::string("Orc No session"));
    closure->Done();
    return nullptr;
  }

  session->set_service_closure(closure);

  if (!session->Init()) {
    ORC_WARN("Session Init fail.");
    closure->set_ret_code(ServiceClosure::RetCode::AppError);
    closure->set_message(std::string("Orc No session"));
    closure->Done();
    session_factory_->Release(session);
    return nullptr;
  }

  auto workflow_name = workflow_router_->Route(session);
  auto exe_graph = ExecutionGraphMgr::Instance()->GetExeGraph(workflow_name);
  if (exe_graph == nullptr) {
    ORC_WARN("Get ExecutionGraph fail by name: %s.", workflow_name.c_str());
    closure->set_ret_code(ServiceClosure::RetCode::AppError);
    closure->set_message(std::string("Get ExecutionGraph fail by name ") + workflow_name);
    closure->Done();
    session_factory_->Release(session);
    return nullptr;
  }

  auto ctx = session->orc_ctx();
  auto th_meta = GetThreadMeta();
  ctx->set_schedule_hint(th_meta->id);
  ctx->Setup(this, exe_graph);

  return ctx;
}

void Workflow::ResetCtx(Context* ctx) {
  auto session = ctx->session();
  auto closure = session->service_closure();
  closure->Done();

  ExecutionGraphMgr::Instance()->ReleaseExeGraph(ctx->graph());
  ctx->Reset();
  session_factory_->Release(session);
}

void Workflow::RunInner(ServiceClosure* closure) {
  auto ctx = SetupCtx(closure);
  if (ctx != nullptr) {
    RunInner(ctx);
  }
}

void Workflow::RunInner(Context* ctx) {
  while (!ctx->IsAsync() && !ctx->IsDone()) {
    ctx->Step();
  }

  if (ctx->IsDone()) {
    ResetCtx(ctx);
  }
}

}  // namespace orc
