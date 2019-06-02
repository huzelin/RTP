#ifndef EXAMPLE_ORC_SERVER_HELLO_SERVER_H_
#define EXAMPLE_ORC_SERVER_HELLO_SERVER_H_

#include "orc/server/brpc_server.h"

namespace orc {
namespace example {
namespace hello {

class HelloServer : public BrpcServer {
 public:
  HelloServer() = default;
  virtual ~HelloServer() = default;

  bool SetupOrcLogger(const YAML::Node& config) override {
    return true;
  }

  bool Init(const YAML::Node& config) override {
    return true;
  }

  google::protobuf::Service* NewService();
};

}  // namespace example
}  // namespace hello
}  // namespace orc


#endif  // EXAMPLE_ORC_SERVER_HELLO_SERVER_H_
