#include "orc/com/component_mgr.h"

#include <list>
#include <algorithm>

#include "orc/util/log.h"
#include "orc/framework/configure.h"

namespace orc {

ComponentMgr* ComponentMgr::Instance() {
  static ComponentMgr mgr;
  return &mgr;
}

void ComponentMgr::Register(const std::string& name, std::unique_ptr<Component> com) {
  components_.emplace(name, std::move(com));
}

bool ComponentMgr::Setup(const YAML::Node& config) {
  std::list<std::string> enable_coms;
  std::list<std::string> disable_coms;

  GetOrcConfig(config, Options::ComEnableList, &enable_coms);
  GetOrcConfig(config, Options::ComDisableList, &disable_coms);

  enable_coms.remove_if([&disable_coms](const std::string& com){
      return std::find(disable_coms.begin(), disable_coms.end(), com) != disable_coms.end();
  });

  for (const auto& com : enable_coms) {
    auto it = components_.find(com);
    if (it == components_.end()) {
      ORC_ERROR("com %s isn't registered.", com.c_str());
      return false;
    }

    if (!it->second->Setup(config)) {
      ORC_ERROR("com %s Setup fail.", com.c_str());
      return false;
    }
    ORC_INFO("com %s Setup success.", com.c_str());
  }

  return true;
}

void ComponentMgr::Release() {
  for (const auto& p : components_) {
    p.second->Release();
  }
}

}  // namespace orc
