#ifndef ORC_PLUGIN_PLUGIN_REGISTER_H_
#define ORC_PLUGIN_PLUGIN_REGISTER_H_

#include "orc/plugin/plugin_info.h"

#define ORC_PLUGIN_DECLARE(ModuleName, Name) \
static orc::PluginInfo* orc_plugin_info_declare_ ## ModuleName ## Name() { \
  static orc::PluginInfo pi; \
  pi.name = #Name; \
  pi.module_name = #ModuleName;

#define ORC_PLUGIN_ADD_TAG(Tag) \
  pi.tags.emplace_back(#Tag);

#define ORC_PLUGIN_ADD_INTERFACE(Name, Interface) \
  pi.interfaces[#Name] = Interface;

#define ORC_PLUGIN_END(ModuleName, Name) \
  return &pi; \
} \
extern "C" { \
orc::PluginInfo* OrcPluginEntryFunc() { \
  return orc_plugin_info_declare_ ## ModuleName ## Name(); \
} \
}

#endif  // ORC_PLUGIN_PLUGIN_REGISTER_H_
