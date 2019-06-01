#ifndef ORC_COM_RPC_CLIENT_RPC_CLIENT_H_
#define ORC_COM_RPC_CLIENT_RPC_CLIENT_H_

#include <string>

#include "orc/util/macros.h"
#include "yaml-cpp/node/node.h"

#include "google/protobuf/service.h"
#include "google/protobuf/message.h"

namespace orc {

class RpcClient {
 public:
  enum class Type {Arpc, Erpc};

  explicit RpcClient(const std::string& name, Type type, int32_t id);
  virtual ~RpcClient();


  virtual bool Init(const YAML::Node& config);

  virtual google::protobuf::RpcChannel* GetChannel() = 0;

  virtual void FreeChannel(google::protobuf::RpcChannel* channel) = 0;

  virtual google::protobuf::RpcController* GetController() = 0;
  virtual void FreeController(google::protobuf::RpcController* controller) = 0;

  virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
                          const google::protobuf::Message* request,
                          google::protobuf::Message* response,
                          google::protobuf::Closure* closure,
                          bool* success,
                          int64_t hint_key = -1);

  void WarmUpChannel();

  const std::string& name() const { return name_; }

  Type type() const { return type_; }
  int32_t id() const { return id_; }

 protected:
  std::string name_;
  Type type_;
  int32_t id_;
  std::string sk_server_path_;
  std::string sk_local_section_name_;
  std::string sk_snapshot_dir_;
  uint32_t sk_rpc_queue_size_;
  uint32_t sk_request_timeout_ms_;
  uint32_t sk_reload_interval_s_;

 public:
  bool sk_dynamic_load_balance_;
  uint32_t sk_dlb_refresh_interval_;
  int32_t sk_dlb_proportional_factor_;
  int32_t sk_dlb_weight_variance_;
  int64_t sk_dlb_max_drop_rate_;
  int64_t sk_dlb_drop_recover_rate_;
  int64_t sk_dlb_drop_recover_pace_;

 private:
  ORC_DISALLOW_COPY_AND_ASSIGN(RpcClient);
};

}  // namespace orc

#endif  // ORC_COM_RPC_CLIENT_RPC_CLIENT_H_
