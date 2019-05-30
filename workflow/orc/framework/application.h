#ifndef ORC_FRAMEWORK_APPLICATION_H_
#define ORC_FRAMEWORK_APPLICATION_H_

#include <queue>
#include <string>
#include <chrono>
#include <map>
#include <memory>

#include "orc/util/macros.h"
#include "orc/framework/server_base.h"

namespace orc {

class Application {
 public:
  Application() = default;
  ~Application() = default;

  bool Run(int argc, char** argv);

  enum class Command {Start, Stop};

 private:
  void Usage(char* exe);
  bool Daemon();
  bool Start();
  bool Stop();
  bool ParseCmdArgs(int argc, char** argv);

  uint64_t GetAppPid();
  bool CheckPid(uint64_t pid);
  void SaveAppPid(uint64_t pid);

  bool NeedRestart(uint64_t pid, int status);
  bool StartServers();
  uint64_t StartServer(const std::string& name, const YAML::Node& config);

  void SuperviseServers();

 private:
  std::string config_file_;
  Command command_;

  struct ServerMeta {
    std::string name;
    uint64_t pid;
    uint32_t intensity;
    uint32_t period;
    YAML::Node config;
    std::queue<std::chrono::time_point<std::chrono::system_clock>> dead_times;
  };

  std::map<uint64_t, std::unique_ptr<ServerMeta>> server_metas_;

  ORC_DISALLOW_COPY_AND_ASSIGN(Application);
};

}  // namespace orc

#endif  // ORC_FRAMEWORK_APPLICATION_H_
