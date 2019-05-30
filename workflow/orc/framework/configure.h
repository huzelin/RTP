#ifndef ORC_FRAMEWORK_CONFIGURE_H_
#define ORC_FRAMEWORK_CONFIGURE_H_

#include <string>

#include "orc/util/macros.h"
#include "orc/util/utils.h"
#include "orc/framework/options.h"

#include "yaml-cpp/yaml.h"

namespace orc {

YAML::Node& OrcConfig();

bool InitOrcConfig(const std::string& file);

template<typename T>
bool GetOrcConfig(const std::string& key, T* value) {
  YAML::Node& config = OrcConfig();
  return GetConfig<T>(config, key, value);
}

template<typename T>
bool GetOrcConfig(const YAML::Node& config, const std::string& key, T* value) {
  if (!GetConfig<T>(config, key, value)) {
    return GetOrcConfig<T>(key, value);
  }
  return true;
}

}  // namespace orc

#define ORC_CONFIG_OR_FAIL(config, item, field) \
do { \
  if (!orc::GetOrcConfig(config, item, &field)) { \
    ORC_ERROR("Not found config item: %s.", item.c_str()); \
    return false; \
  } \
} while (0)

#define ORC_CONFIG_OR_DEFAULT(config, item, field, def) \
do { \
  if (!orc::GetOrcConfig(config, item, &field)) { \
    ORC_DEBUG("Not found config item: %s, use default.", item.c_str()); \
    field = def; \
  } \
} while (0)


#endif  // ORC_FRAMEWORK_CONFIGURE_H_
