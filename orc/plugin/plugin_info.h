#ifndef ORC_PLUGIN_PLUGIN_INFO_H_
#define ORC_PLUGIN_PLUGIN_INFO_H_

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace orc {

using PluginInterface = void*(*)(void*);

struct PluginInfo {
  std::string name;
  std::string module_name;
  std::vector<std::string> tags;
  std::map<std::string, PluginInterface> interfaces;
};

// Plugin's the only interface that interacts with the framework.
// This interface return a pointor to one instance of PluginInfo. The plugin
// must ensure this pointor is valid until it is destroyed.
static const std::string kPluginEntryFuncName = "OrcPluginEntryFunc";

using PluginEntryFunc = PluginInfo*(*)();

struct ObjectInfo {
  std::string name;
  std::string name_space;
  std::function<void*(void*)> creator;
  std::function<void(void*)> deleter;
  std::string lifecycle;
};

}  // namespace orc

#endif  // ORC_PLUGIN_PLUGIN_INFO_H_
