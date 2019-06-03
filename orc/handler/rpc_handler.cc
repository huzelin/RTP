#include "orc/handler/rpc_handler.h"

#include <atomic>

#include "orc/util/utils.h"
#include "orc/util/log.h"
#include "orc/util/closure.h"

#include "orc/com/rpc_client/rpc_client_factory.h"

namespace orc {

bool RpcHandlerBase::BaseInit(const YAML::Node& config) {
  std::string method_name;
  CONFIG_OR_FAIL(config, "rpc_method", method_name);

  service_name_ = service_descriptor()->name();
  method_ = service_descriptor()->FindMethodByName(method_name);
  if (method_ == nullptr) {
    ORC_ERROR("Service: %s doesn't have a method: %s.",
              service_name_.c_str(), method_name.c_str());
    return false;
  }

  return Init(config);
}

namespace {

class RpcInvoker : public Closure {
 public:
  explicit RpcInvoker(Context* context) : context_(context) {
    context->set_async_closure(this);
  }
  virtual ~RpcInvoker() = default;

  std::vector<RpcParam>* mutable_rpc_param() { return &rpc_param_; }
  const std::vector<RpcParam>& rpc_param() const { return rpc_param_; }

  bool Invoke(const std::string& service_name,
              const google::protobuf::MethodDescriptor* method);

 private:
  std::vector<RpcParam> rpc_param_;
  Context* context_;
};

class BarrierClosure : public google::protobuf::Closure {
 public:
  explicit BarrierClosure(uint32_t times, Context* context)
      : times_(times), context_(context) {}
  virtual ~BarrierClosure() = default;

  void Run() {
    if (--times_ != 0) return;
    context_->Callback();
    delete this;
  }

 private:
  std::atomic<uint32_t> times_;
  Context* context_;
};

bool RpcInvoker::Invoke(const std::string& service_name,
                        const google::protobuf::MethodDescriptor* method) {
  auto client_group = RpcClientFactory::Instance()->GetRpcClient(service_name);
  if (client_group == nullptr) {
    ORC_ERROR("GetClientGroup for %s fail.", service_name.c_str());
    context_->SetAsync(false);
    return false;
  }

  for (const RpcParam& param : rpc_param_) {
    if (param.resps.size() != client_group->size()) {
      ORC_ERROR("Response size: %zu != ClientGroup size: %zu.",
                param.resps.size(), client_group->size());
      context_->SetAsync(false);
      return false;
    }
  }

  size_t call_times = rpc_param_.size() * client_group->size();
  auto barrier_closure = new BarrierClosure(call_times, context_);

  // Async call start.
  context_->SetAsync(true);
  for (RpcParam& param : rpc_param_) {
    for (size_t i = 0; i < client_group->size(); ++i) {
      (*client_group)[i]->CallMethod(
          method, param.request, param.resps[i].response, barrier_closure,
          &param.resps[i].success);
    }
  }

  return true;
}

}  // anonymous namespace

bool RpcHandlerBase::BaseRun(SessionBase* session_base, Context* context) {
  if (context->async_state() == Context::AsyncState::Before) {
    auto rpc_invoker = new RpcInvoker(context);
    if (!BuildRequest(session_base, rpc_invoker->mutable_rpc_param())) {
      context->SetAsync(false);
      return false;
    }

    if (rpc_invoker->rpc_param().size() == 0) {
      context->SetAsync(false);
      return true;
    }

    return rpc_invoker->Invoke(service_name_, method_);

  } else if (context->async_state() == Context::AsyncState::After) {
    context->set_async_state(Context::AsyncState::Finish);
    auto rpc_invoker = static_cast<RpcInvoker*>(context->async_closure());

    return ProcessResponse(session_base, rpc_invoker->rpc_param());

  } else {
    // Never reach here.
    ORC_ERROR("RpcHandlerBase reached the impossible path");
    return false;
  }
}

}  // namespace orc
