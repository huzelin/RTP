#include "orc/framework/configure.h"

#include <thread>

#include "orc/util/log.h"

namespace orc {

YAML::Node& OrcConfig() {
  static YAML::Node node;
  return node;
}

static void DefaultConfig() {
#define DEFAULT_CONFIG(key, value) config[key] = value

  YAML::Node& config = OrcConfig();

  DEFAULT_CONFIG(Options::OrcDaemon,          true);
  DEFAULT_CONFIG(Options::OrcInFile,          "/dev/null");
  DEFAULT_CONFIG(Options::OrcOutFile,         "/dev/null");
  DEFAULT_CONFIG(Options::OrcErrFile,         "/dev/null");
  DEFAULT_CONFIG(Options::OrcLogMaxLength,    1024);
  DEFAULT_CONFIG(Options::OrcLogLevel,        "info");

  DEFAULT_CONFIG(Options::SvrIP,                      "0.0.0.0");
  DEFAULT_CONFIG(Options::SvrMaxRestartIntensity,     5);
  DEFAULT_CONFIG(Options::SvrMaxRestartPeriod,        300);
  DEFAULT_CONFIG(Options::SvrIoThreadNum,             4);
  DEFAULT_CONFIG(Options::SvrWorkflowPoolMode,        "pool");

  DEFAULT_CONFIG(Options::SvrWorkerNum, std::thread::hardware_concurrency());
  DEFAULT_CONFIG(Options::SvrWorkerQueueNum, 1);
  DEFAULT_CONFIG(Options::SvrWorkerQueueSize, 1024);
  DEFAULT_CONFIG(Options::SvrMonitorStatusFile, "/tmp/monitor_status.dat");

#undef DEFAULT_CONFIG
}

bool InitOrcConfig(const std::string& file) {
  DefaultConfig();

  YAML::Node& config = OrcConfig();
  try {
    YAML::Node node = YAML::LoadFile(file);
    for (auto it = node.begin(); it != node.end(); ++it) {
      config[it->first.as<std::string>()] = it->second;
    }
    return true;
  } catch (const YAML::Exception& e) {
    ORC_ERROR("YAML load file: %s fail. for: %s.", file.c_str(), e.what());
    return false;
  }
}

}  // namespace orc
