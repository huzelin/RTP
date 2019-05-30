#ifndef ORC_PLUGIN_PLUGIN_H_
#define ORC_PLUGIN_PLUGIN_H_

#include <string>
#include <vector>

#include "orc/plugin/plugin_info.h"

namespace orc {

class Plugin {
 public:
  Plugin();
  virtual ~Plugin();

  bool Init(const std::string& path);

  PluginInterface GetInterface(const std::string& name);

  const PluginInfo* plugin_info() const { return plugin_info_; }
  const std::string& path() const { return path_; }

 private:
  std::string path_;
  PluginInfo* plugin_info_;
  void* raw_lib_;
};

}  // namespace orc

#endif  // ORC_PLUGIN_PLUGIN_H_
