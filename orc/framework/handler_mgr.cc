#include "orc/framework/handler_mgr.h"

#include "orc/util/log.h"
#include "orc/framework/configure.h"

namespace orc {

HandlerMgr* HandlerMgr::Instance() {
  static HandlerMgr mgr;
  return &mgr;
}

bool HandlerMgr::Setup(const YAML::Node& config) {
  std::string config_file;
  ORC_CONFIG_OR_FAIL(config, Options::SvrHandlerFile, config_file);
  return ReloadConfigFile(config_file, false);
}

void HandlerMgr::Register(const std::string& name,
                          HandlerMgr::HandlerCtor ctor) {
  handler_ctors_[name].emplace_back(std::move(ctor));
}

bool HandlerMgr::ReloadConfigFile(const std::string& file, bool need_check) {
  YAML::Node config;
  try {
     config = YAML::LoadFile(file);
  } catch (const YAML::Exception& e) {
    ORC_ERROR("YAML load file: %s fail. for: %s.", file.c_str(), e.what());
    return false;
  }

  if (need_check) {
    for (const auto& p : handler_ctors_) {
      if (p.second.size() > 1) {
        ORC_ERROR("Handler: %s register multi-times.", p.first.c_str());
        return false;
      }

      std::unique_ptr<HandlerBase> handler{p.second[0]()};
      const YAML::Node& hconf = config[p.first] ? config[p.first] : YAML::Node();

      if (!handler->BaseInit(hconf)) {
        ORC_ERROR("Handler: %s init fail.", p.first.c_str());
        return false;
      }
    }
  }

  // Update config
  handler_configs_ = config;
  config_file_ = file;
  ORC_INFO("HandlerMgr Setup successfully.");
  return true;
}

std::unique_ptr<HandlerBase> HandlerMgr::GetHandler(const std::string& name) {
  auto ctor = handler_ctors_.find(name);
  if (ctor == handler_ctors_.end()) {
    ORC_ERROR("Hanlder: %s not register.", name.c_str());
    return nullptr;
  }

  std::unique_ptr<HandlerBase> handler{ctor->second[0]()};

  const YAML::Node& conf = handler_configs_[name] ? handler_configs_[name] : YAML::Node();
  if (!handler->BaseInit(conf)) {
    ORC_ERROR("Handler: %s BaseInit fail.", name.c_str());
    return nullptr;
  }

  handler->set_name(name);
  return handler;
}

}  // namespace orc
