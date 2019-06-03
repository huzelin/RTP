#include "orc/com/rpc_client/rpc_client.h"

#include <sys/time.h>
#include <unistd.h>

#include "orc/util/log.h"
#include "orc/util/utils.h"

#include "common/monitor/monitor_status_impl.h"

namespace orc {

RpcClient::RpcClient(const std::string& name, Type type, int32_t id = 0)
  : name_(name),
    type_(type),
    id_(id) {}

RpcClient::~RpcClient() {}

bool RpcClient::Init(const YAML::Node& config) {
  CONFIG_OR_FAIL(config, "leader_server_path", leader_server_path_);
  CONFIG_OR_FAIL(config, "leader_request_timeout", leader_request_timeout_ms_);
  CONFIG_OR_FAIL(config, "leader_channel_count", leader_channel_count_);
  CONFIG_OR_FAIL(config, "leader_hb_interval", leader_hb_interval_s_);
  return true;
}

namespace {

class RpcClosure : public google::protobuf::Closure {
 public:
  RpcClosure(RpcClient* client,
             google::protobuf::RpcChannel* channel,
             google::protobuf::RpcController* controller,
             google::protobuf::Closure* closure,
             const google::protobuf::Message* request,
             const google::protobuf::Message* response,
             bool* success)
      : client_(client),
        channel_(channel),
        controller_(controller),
        closure_(closure),
        request_(request),
        response_(response),
        success_(success) {
    gettimeofday(&start_time_, NULL);
  }

  ~RpcClosure() {
    client_->FreeChannel(channel_);
    client_->FreeController(controller_);
    MONITOR_STATUS_NORMAL_TIMER_BY(client_->name().c_str(), "all", EscapeTime(), 1);
  }

  virtual void Run() {
    MONITOR_STATUS_NORMAL_TIMER_BY(client_->name().c_str(), "cb_start", EscapeTime(), 1);

    if (controller_->Failed()) {
      *success_ = false;
      ORC_WARN("Rpc Call for (%s) fail for: %s.",
               client_->name().c_str(), controller_->ErrorText().c_str());
      MONITOR_STATUS_NORMAL_TIMER_BY(client_->name().c_str(), "cb_fail", EscapeTime(), 1);
      MONITOR_STATUS_NORMAL_TIMER_BY(client_->name().c_str(), "cb_fail_rate", 100 * 1000, 1);
    } else {
      *success_ = true;
      Benchmark();
      MONITOR_STATUS_NORMAL_TIMER_BY(client_->name().c_str(), "cb_fail_rate", 0 * 1000, 1);
    }

    closure_->Run();

    int64_t time_stamp;
    int64_t rt_usec = EscapeTime(&time_stamp);
    MONITOR_STATUS_NORMAL_TIMER_BY(
        client_->name().c_str(), "cb_after_user_cb", rt_usec, 1);
    int64_t rt_msec = rt_usec / 1000;
    delete this;
  }

  int64_t EscapeTime(int64_t* timestamp = nullptr) const {
    struct timeval now;
    gettimeofday(&now, NULL);

    int64_t escape = (now.tv_sec - start_time_.tv_sec) * 1000000;
    escape += now.tv_usec - start_time_.tv_usec;
    if (timestamp != nullptr) {
      *timestamp = now.tv_sec;
    }
    return escape;
  }

  void Benchmark() {
    MONITOR_STATUS_NORMAL_TIMER_BY(client_->name().c_str(), "req_size",
                                   request_->ByteSize() * 1000, 1);

    MONITOR_STATUS_NORMAL_TIMER_BY(client_->name().c_str(), "res_size",
                                   response_->ByteSize() * 1000, 1);
  }

 private:
  RpcClient* client_;
  google::protobuf::RpcChannel* channel_;
  google::protobuf::RpcController* controller_;
  google::protobuf::Closure* closure_;
  const google::protobuf::Message* request_;
  const google::protobuf::Message* response_;
  bool* success_;
  struct timeval start_time_;
};

}  // anonymous namespace

void RpcClient::CallMethod(const google::protobuf::MethodDescriptor* method,
                           const google::protobuf::Message* request,
                           google::protobuf::Message* response,
                           google::protobuf::Closure* closure,
                           bool* success) {
  google::protobuf::RpcChannel* channel = nullptr;
  channel = GetChannel();

  if (channel == nullptr) {
    ORC_WARN("Get Rpc channel fail for service: %s.", name_.c_str());
    MONITOR_STATUS_NORMAL_TIMER_BY(
        name_.c_str(), "no_channel_rate", 100 * 1000, 1);
    if (closure != nullptr) closure->Run();
    return;
  }
  MONITOR_STATUS_NORMAL_TIMER_BY(
      name_.c_str(), "no_channel_rate", 0, 1);

  google::protobuf::RpcController* ctrl = GetController();
  if (closure == nullptr) {
    // sync
    channel->CallMethod(method, ctrl, request, response, nullptr);
    if (!ctrl->Failed()) {
      *success = true;
    } else {
      ORC_WARN("Rpc Call for (%s) fail for: %s.",
               name().c_str(), ctrl->ErrorText().c_str());
    }
    FreeController(ctrl);
    FreeChannel(channel);
  } else {
    // async
    RpcClosure* rpc_closure = new RpcClosure(this, channel, ctrl, closure,
                                             request, response, success);
    channel->CallMethod(method, ctrl, request, response, rpc_closure);
  }
}

void RpcClient::WarmUpChannel() {
  sleep(1);
  for (int32_t i = 0; i < 500; ++i) {
    GetChannel();
  }
}

}  // namespace orc
