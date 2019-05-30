#include "orc/plugin/plugin_loader.h"

#include <dlfcn.h>

namespace orc {

bool PluginLoader::Load(const std::string& path, void** plugin, std::string* err_msg) {
  *plugin = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (*plugin == nullptr) {
    err_msg->assign(dlerror());
    return false;
  }
  return true;
}

bool PluginLoader::FindSymbol(void* plugin, const std::string& name, void** symbol,
                              std::string* err_msg) {
  if (plugin == nullptr) {
    err_msg->assign("plugin is nullptr.");
    return false;
  }

  dlerror();  // for clear old error conditions.
  *symbol = dlsym(plugin, name.c_str());
  auto err = dlerror();  // for check dlsym whether success.
  if (err != nullptr) {
    err_msg->assign(err);
    return false;
  }
  return true;
}

bool PluginLoader::Close(void* plugin, std::string* err_msg) {
  if (plugin == nullptr) return true;

  if (dlclose(plugin) != 0) {
    err_msg->assign(dlerror());
    return false;
  }
  return true;
}

}  // namespace orc
