#ifndef ORC_FRAMEWORK_SERVER_BASE_H_
#define ORC_FRAMEWORK_SERVER_BASE_H_

#include <atomic>

#include "orc/util/macros.h"
#include "yaml-cpp/yaml.h"
#include "orc/util/log_pub.h"

namespace orc {

class ServerBase {
 public:
  ServerBase();
  virtual ~ServerBase() = default;

  virtual bool Init(const YAML::Node& config) = 0;

  virtual bool StartServer(const YAML::Node& config) = 0;
  virtual bool StopServer() = 0;
  virtual bool ServicePublish(const YAML::Node& config) = 0;
  virtual bool ServiceHide() = 0;

  virtual bool SetupOrcLogger(const YAML::Node& config);
  virtual bool BaseRun(const YAML::Node& config);
  virtual void BaseStop();

  void set_name(const std::string& name) { name_ = name; }
  const std::string& name() const { return name_; }

 protected:
  enum class Status {Start, Stop};
  Status status_;

 private:
  std::string name_;
  std::atomic<bool> stop_;

  ORC_DISALLOW_COPY_AND_ASSIGN(ServerBase);
};

}  // namespace orc

#endif  // ORC_FRAMEWORK_SERVER_BASE_H_
