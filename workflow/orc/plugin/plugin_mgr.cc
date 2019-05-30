#include "orc/plugin/plugin_mgr.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "orc/util/log.h"
#include "orc/util/scope_guard.h"

#include "yaml-cpp/yaml.h"

namespace orc {

static std::string kPluginConfig = "plugin.yaml";

PluginMgr::PluginMgr() {}

PluginMgr::~PluginMgr() {}

bool PluginMgr::Init(const std::string& path) {
  std::string real_path;
  if (!NormalizePath(path, &real_path)) {
    return false;
  }

  if (IsExsitPluginConf(real_path)) {
    if (!LoadPluginsByConf(real_path)) {
      return false;
    }

  } else if (!LoadPlugins(real_path)) {
    return false;
  }

  BuildIndex();

  ORC_INFO("PluginMgr Init success.");
  return true;
}

bool PluginMgr::NormalizePath(const std::string& path, std::string* norm_path) {
  char buf[PATH_MAX];
  char* real_path = realpath(path.c_str(), buf);
  if (real_path == nullptr) {
    ORC_ERROR("Get real path of path: %s fail.", path.c_str());
    return false;
  }
  norm_path->assign(real_path);
  return true;
}

bool PluginMgr::IsExsitPluginConf(const std::string& path) {
  std::string file = path + "/" + kPluginConfig;
  struct stat st;
  if (stat(file.c_str(), &st) != 0) {
    ORC_DEBUG("plugin config: %s is not exsit.", file.c_str());
    return false;
  }

  if (!S_ISREG(st.st_mode)) {
    ORC_WARN("plugin config: %s is not a regular file.", file.c_str());
    return false;
  }

  ORC_INFO("found plugin config: %s.", file.c_str());
  return true;
}

bool PluginMgr::LoadPluginsByConf(const std::string& path) {
  std::string file = path + "/" + kPluginConfig;

  try {
    YAML::Node node = YAML::LoadFile(file);
    if (!node["plugins"]) {
      ORC_ERROR("Can't get option plugins from plugin config: %s.", file.c_str());
      return false;
    }

    YAML::Node plugins = node["plugins"];
    if (!plugins.IsSequence()) {
      ORC_ERROR("plugins option from plugin config: %s is not in sequence format.", file.c_str());
      return false;
    }

    for (size_t i = 0; i < plugins.size(); ++i) {
      std::string plugin_path = path + "/" + plugins[i].as<std::string>();
      if (!LoadPlugins(plugin_path)) {
        return false;
      }
    }

    return true;
  } catch (const YAML::Exception& e) {
    ORC_ERROR("Load plugin config: %s fail. for: %s.", file.c_str(), e.what());
    return false;
  }
}

bool PluginMgr::LoadPlugins(const std::string& path) {
  struct stat st;
  if (stat(path.c_str(), &st) != 0) {
    ORC_ERROR("Get file info of path: %s fail.", path.c_str());
    return false;
  }

  if (S_ISDIR(st.st_mode)) {
    return LoadDirectory(path);
  }

  if (S_ISREG(st.st_mode)) {
    return LoadFile(path);
  }

  ORC_ERROR("Unkown file type of file: %s.", path.c_str());
  return false;
}

bool PluginMgr::LoadFile(const std::string& file) {
  if ((file.size() <= 3) || (strncmp(file.c_str() + file.size() - 3, ".so", 3) != 0)) {
    ORC_DEBUG("file: %s is not a dynamic lib.", file.c_str());
    return true;
  }

  std::unique_ptr<Plugin> plugin(new Plugin());
  if (!plugin->Init(file)) {
    ORC_ERROR("plugin: %s Init fail.", file.c_str());
    return false;
  }

  plugins_.emplace_back(std::move(plugin));
  ORC_INFO("plugin: %s load success.", file.c_str());
  return true;
}

bool PluginMgr::LoadDirectory(const std::string& dir) {
  DIR* d = opendir(dir.c_str());
  if (d == nullptr) {
    ORC_ERROR("open dir: %s fail.", dir.c_str());
    return false;
  }

  auto close_d_guard = MakeScopeGuard([d](){ closedir(d); });

  struct dirent* ent;
  while ((ent = readdir(d)) != nullptr) {
    auto len = strlen(ent->d_name);
    if (((len == 1) && (ent->d_name[0] == '.')) ||
        ((len == 2) && (ent->d_name[0] == '.') && (ent->d_name[1] == '.'))) {
      continue;
    }

    if (!LoadPlugins(dir + '/' + ent->d_name)) {
      return false;
    }
  }
  return true;
}

void PluginMgr::BuildIndex() {
  for (const auto& plugin : plugins_) {
    auto pi = plugin->plugin_info();
    auto rawp = plugin.get();

    name_idx_[pi->name].emplace_back(rawp);
    module_idx_[pi->module_name].emplace_back(rawp);

    for (const auto& tag : pi->tags) {
      tag_idx_[tag].emplace_back(rawp);
      module_tag_idx_[pi->module_name][tag].emplace_back(rawp);
    }
  }
}

std::vector<Plugin*> PluginMgr::FindPluginByName(const std::string& name) const {
  auto it = name_idx_.find(name);
  if (it == name_idx_.end()) return {};
  return it->second;
}

std::vector<Plugin*> PluginMgr::FindPluginByModule(const std::string& module) const {
  auto it = module_idx_.find(module);
  if (it == module_idx_.end()) return {};
  return it->second;
}

std::vector<Plugin*> PluginMgr::FindPluginByTag(const std::string& tag) const {
  auto it = tag_idx_.find(tag);
  if (it == tag_idx_.end()) return {};
  return it->second;
}

std::vector<Plugin*> PluginMgr::FindPluginByModuleName(
    const std::string& module, const std::string& name) const {
  std::vector<Plugin*> plugins;
  auto it = name_idx_.find(name);
  if (it == name_idx_.end()) return plugins;

  for (auto& p : it->second) {
    if (p->plugin_info()->module_name == module) {
      plugins.emplace_back(p);
    }
  }
  return plugins;
}

std::vector<Plugin*> PluginMgr::FindPluginByModuleTag(
    const std::string& module, const std::string& tag) const {
  std::vector<Plugin*> plugins;
  auto it = module_tag_idx_.find(module);
  if (it == module_tag_idx_.end()) return plugins;

  auto itt = it->second.find(tag);
  if (itt == it->second.end()) return plugins;
  return itt->second;
}

std::vector<Plugin*> PluginMgr::FindPluginByModuleTagName(
    const std::string& module, const std::string& tag, const std::string& name) const {
  std::vector<Plugin*> plugins;
  auto it = module_tag_idx_.find(module);
  if (it == module_tag_idx_.end()) return plugins;

  auto itt = it->second.find(tag);
  if (itt == it->second.end()) return plugins;
  for (auto& p : itt->second) {
    if (p->plugin_info()->name == name) {
      plugins.emplace_back(p);
    }
  }
  return plugins;
}

}  // namespace orc
