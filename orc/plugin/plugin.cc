#include "orc/plugin/plugin.h"

#include "orc/plugin/plugin_loader.h"
#include "orc/util/log.h"

namespace orc {

Plugin::Plugin() : plugin_info_(nullptr), raw_lib_(nullptr) {}

Plugin::~Plugin() {
  if (raw_lib_ != nullptr) {
    std::string err_mgs;
    if (!PluginLoader::Close(raw_lib_, &err_mgs)) {
      if (plugin_info_ != nullptr) {
        ORC_ERROR("Close plugin: %s fail, reason: %s.", plugin_info_->name.c_str(),
                  err_mgs.c_str());
      } else {
        ORC_ERROR("Close plugin: %p fail, reason: %s.", raw_lib_,
                  err_mgs.c_str());
      }
      return;
    }
    raw_lib_ = nullptr;
  }
}

bool Plugin::Init(const std::string& path) {
  path_ = path;

  std::string err_msg;
  if (!PluginLoader::Load(path_, &raw_lib_, &err_msg)) {
    ORC_ERROR("Load plugin: %s fail, reason: %s.", path.c_str(), err_msg.c_str());
    return false;
  }

  PluginInfo* (*entry_func)();
  if (!PluginLoader::FindSymbol(raw_lib_, kPluginEntryFuncName,
                                (void**)&entry_func, &err_msg)) {
    ORC_ERROR("Find plugin: %s entry: %s fail, reason: %s.",
              path.c_str(), kPluginEntryFuncName.c_str(), err_msg.c_str());
    return false;
  }

  plugin_info_ = entry_func();
  if (plugin_info_ == nullptr) {
    ORC_ERROR("The PluginInfo of plugin: %s is null.", path.c_str());
    return false;
  }

  ORC_DEBUG("Plugin: %s Init success.", path_.c_str());
  return true;
}

PluginInterface Plugin::GetInterface(const std::string& name) {
  if (plugin_info_ == nullptr) {
    ORC_ERROR("The plugin_info_ is nullptr.");
    return nullptr;
  }

  auto it = plugin_info_->interfaces.find(name);
  if (it == plugin_info_->interfaces.end()) {
    return nullptr;
  }
  return it->second;
}

}  // namespace orc
