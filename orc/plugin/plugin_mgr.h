#ifndef ORC_PLUGIN_PLUGIN_MGR_H_
#define ORC_PLUGIN_PLUGIN_MGR_H_

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "orc/plugin/plugin_info.h"
#include "orc/plugin/plugin.h"

namespace orc {

class PluginMgr {
 public:
  PluginMgr();
  ~PluginMgr();

  bool Init(const std::string& path);

  std::vector<Plugin*> FindPluginByName(const std::string& name) const;
  std::vector<Plugin*> FindPluginByModule(const std::string& module) const;
  std::vector<Plugin*> FindPluginByTag(const std::string& tag) const;

  std::vector<Plugin*> FindPluginByModuleName(
      const std::string& module, const std::string& name) const;

  std::vector<Plugin*> FindPluginByModuleTag(
      const std::string& module, const std::string& tag) const;

  std::vector<Plugin*> FindPluginByModuleTagName(
      const std::string& module, const std::string& tag, const std::string& name) const;

 private:
  bool NormalizePath(const std::string& path, std::string* norm_path);
  bool LoadPlugins(const std::string& dir);
  bool LoadFile(const std::string& file);
  bool LoadDirectory(const std::string& dir);

  bool IsExsitPluginConf(const std::string& path);
  bool LoadPluginsByConf(const std::string& file);

  void BuildIndex();

 private:
  std::vector<std::unique_ptr<Plugin>> plugins_;

  std::map<std::string, std::vector<Plugin*>> name_idx_;
  std::map<std::string, std::vector<Plugin*>> module_idx_;
  std::map<std::string, std::vector<Plugin*>> tag_idx_;
  std::map<std::string, std::map<std::string, std::vector<Plugin*>>> module_tag_idx_;
};

}  // namespace orc

#endif  // ORC_PLUGIN_PLUGIN_MGR_H_
