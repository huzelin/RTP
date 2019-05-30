#ifndef ORC_UTIL_UTILS_H_
#define ORC_UTIL_UTILS_H_

#include <string>
#include "yaml-cpp/yaml.h"

#include "orc/util/log.h"
#include "orc/util/thread_pool.h"

namespace orc {

template<typename T>
bool GetConfig(const YAML::Node& config, const std::string& key, T* value) {
  if (config[key]) {
    try {
      *value = config[key].as<T>();
      return true;
    } catch (...) {
      return false;
    }
  }
  return false;
}

#define CONFIG_OR_FAIL(config, item, field) \
do { \
  if (!orc::GetConfig(config, item, &field)) { \
    ORC_ERROR("Not found config item: %s.", item); \
    return false; \
  } \
} while (0)

#define CONFIG_OR_DEFAULT(config, item, field, def) \
do { \
  if (!orc::GetConfig(config, item, &field)) { \
    ORC_DEBUG("Not found config item: %s, use default", item); \
    field = def; \
  } \
} while (0)

struct ThreadMeta {
  bool init;
  uint32_t rand;  // Random generator's status.
  ThreadPool* pool;  // Which ThreadPool of current thread belong.
  uint32_t id;  // The id indexes in the ThreadPool.
  int tid;  // Come from linux's gettid system call for debug with commands like
            // 'pstack', 'top'.
  char tid_str[32];  // String of tid.
};

ThreadMeta* GetThreadMeta();

// Linux system thread id of current thread.
int tid();

// String of linux system thread id of current thread.
char* tid_str();

// Thread safe random generator.
uint32_t Random();

uint32_t RandomMax();

}  // namespace orc

#endif  // ORC_UTIL_UTILS_H_
