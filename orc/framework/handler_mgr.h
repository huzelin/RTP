#ifndef ORC_FRAMEWORK_HANDLER_MGR_H_
#define ORC_FRAMEWORK_HANDLER_MGR_H_

#include <memory>
#include <map>
#include <vector>
#include <string>

#include "orc/util/macros.h"
#include "orc/framework/handler_base.h"

#include "yaml-cpp/yaml.h"

namespace orc {

class HandlerMgr {
 public:
  ~HandlerMgr() = default;
  static HandlerMgr* Instance();

  // Setup can be called multi-times, echo call update the config.
  bool Setup(const YAML::Node& config);

  using HandlerCtor = std::function<HandlerBase*()>;
  void Register(const std::string& name, HandlerCtor ctor);

  std::unique_ptr<HandlerBase> GetHandler(const std::string& name);

 private:
  HandlerMgr() = default;
  bool ReloadConfigFile(const std::string& handler_file, bool need_check);

 private:
  std::map<std::string, std::vector<HandlerCtor>> handler_ctors_;
  std::string config_file_;
  YAML::Node handler_configs_;

  ORC_DISALLOW_COPY_AND_ASSIGN(HandlerMgr);
};

}  // namespace orc

#define ORC_REGISTER_HANDLER(Handler) \
__attribute__((constructor)) void orc_register_handler_##Handler() { \
  orc::HandlerMgr::Instance()->Register( \
      #Handler, [](){return new Handler();}); \
}

#endif  // ORC_FRAMEWORK_HANDLER_MGR_H_
