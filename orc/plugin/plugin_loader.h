#ifndef ORC_PLUGIN_PLUGIN_LOADER_H_
#define ORC_PLUGIN_PLUGIN_LOADER_H_

#include <string>

namespace orc {

class PluginLoader {
 public:
  static bool Load(const std::string& path, void** plugin, std::string* err_msg);

  static bool FindSymbol(void* plugin, const std::string& name, void** symbol,
                         std::string* err_msg);

  static bool Close(void* plugin, std::string* err_msg);
};

}  // namespace orc

#endif  // ORC_PLUGIN_PLUGIN_LOADER_H_
