#include "orc/framework/application.h"

#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>

#include <iostream>  // NOLINT
#include <fstream>  // NOLINT
#include <thread>
#include <chrono>

#include "orc/util/log.h"
#include "orc/util/scope_guard.h"
#include "orc/framework/configure.h"
#include "orc/framework/server_mgr.h"

namespace orc {

void Application::Usage(char* exe) {
  ORC_INFO("Usage: %s -c <config> start|stop", exe);
}

bool Application::Daemon() {
  pid_t child;
  if ((child = fork()) ==  -1) {
    ORC_ERROR("fork fail.");
    return false;
  }

  if (child > 0) {
    exit(0);
  }

  setsid();

  if ((child = fork()) ==  -1) {
    ORC_ERROR("fork fail.");
    return false;
  }

  if (child > 0) {
    exit(0);
  }

  signal(SIGPIPE, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);

  for (int i = sysconf(_SC_OPEN_MAX); i >= 0; --i) {
    close(i);
  }

  std::string infile = "/dev/null";
  std::string outfile = "/dev/null";
  std::string errfile = "/dev/null";

  GetOrcConfig(Options::OrcInFile, &infile);
  GetOrcConfig(Options::OrcOutFile, &outfile);
  GetOrcConfig(Options::OrcErrFile, &errfile);

  freopen(infile.c_str(), "r", stdin);
  freopen(outfile.c_str(), "a", stdout);
  freopen(errfile.c_str(), "a", stderr);
  return true;
}

uint64_t Application::GetAppPid() {
  std::string path = config_file_.substr(0, config_file_.find_last_of('/'));
  std::ifstream ifs(path+"/.pid");
  uint64_t last_pid = 0;
  ifs >> last_pid;
  return last_pid;
}

bool Application::CheckPid(uint64_t pid) {
  // ((pid == 0) || (kill(pid, 0) != 0))
  if (pid == 0) return false;

  char path[PATH_MAX];
  snprintf(path, PATH_MAX, "/proc/%lu", pid);
  struct stat st;
  return (stat(path, &st) == 0);
}

void Application::SaveAppPid(uint64_t pid) {
  std::string path = config_file_.substr(0, config_file_.find_last_of('/'));
  std::ofstream ofs(path+"/.pid");
  ofs << pid;
}

static void SvrSignalHandler(int sig) {
  if (sig == SIGUSR1) {
    ServerMgr::Instance()->StopServer();
  }
}

uint64_t Application::StartServer(const std::string& name, const YAML::Node& config) {
  pid_t pid = fork();
  if (pid == 0) {
    signal(SIGUSR1, SvrSignalHandler);
    ServerMgr::Instance()->StartServer(name, config);
    exit(0);
  } else if (pid > 0) {
    return pid;
  } else {
    return 0;  // Index error.
  }
}

bool Application::StartServers() {
  YAML::Node& orc_config = OrcConfig();
  if (!orc_config["servers"]) {
    ORC_ERROR("Can't get option: servers from config.");
    return false;
  }

  YAML::Node servers_config = orc_config["servers"];
  if (!servers_config.IsSequence()) {
    ORC_ERROR("servers section in config is not sequence format.");
    return false;
  }
  for (size_t i = 0; i < servers_config.size(); ++i) {
    std::unique_ptr<ServerMeta> meta{new ServerMeta()};

    meta->config = servers_config[i];
    if (!GetConfig(meta->config, Options::SvrName, &(meta->name))) {
      ORC_ERROR("Can't get option \'%s\' from config.", Options::SvrName.c_str());
      return false;
    }

    GetOrcConfig(meta->config, Options::SvrMaxRestartIntensity, &(meta->intensity));
    GetOrcConfig(meta->config, Options::SvrMaxRestartPeriod, &(meta->period));

    meta->pid = StartServer(meta->name, meta->config);
    if (meta->pid == 0) {
      ORC_ERROR("Start server: %s fail.", meta->name.c_str());
      return false;
    }
    server_metas_.emplace(meta->pid, std::move(meta));
  }

  return true;
}

static bool g_is_stop = false;

static void AppSignalHandler(int sig) {
  if (sig == SIGUSR1) {
    static bool from_self = false;
    g_is_stop = true;
    if (!from_self) {
      from_self = true;
      kill(0, SIGUSR1);
    } else {
      from_self = false;
    }
  }
}

bool Application::NeedRestart(uint64_t pid, int status) {
  if (g_is_stop) return false;

  if (WIFEXITED(status)) return false;
  if (WIFSIGNALED(status) && (WTERMSIG(status) == SIGKILL)) return false;

  auto it = server_metas_.find(pid);
  if (it == server_metas_.end()) return false;

  auto now = std::chrono::system_clock::now();
  auto& dead_times = it->second->dead_times;
  dead_times.emplace(now);

  auto start = now - std::chrono::seconds(it->second->period);
  while (!dead_times.empty()) {
    auto point = dead_times.front();
    if (point <= start) dead_times.pop();
    break;
  }

  return (dead_times.size() <= it->second->intensity);
}

void Application::SuperviseServers() {
  while (!server_metas_.empty()) {
    int status;
    pid_t pid_raw = wait(&status);
    if (pid_raw == -1) continue;

    uint64_t pid = static_cast<uint64_t>(pid_raw);
    if (NeedRestart(pid, status)) {
      std::unique_ptr<ServerMeta> meta = std::move(server_metas_[pid]);
      server_metas_.erase(pid);
      meta->pid = StartServer(meta->name, meta->config);
      if (meta->pid != 0) {
        server_metas_.emplace(meta->pid, std::move(meta));
      }
    } else {
      server_metas_.erase(pid);
    }
  }
}

bool Application::Start() {
  uint64_t last_pid = GetAppPid();
  // if ((last_pid != 0) && (kill(last_pid, 0) == 0)) {
  if (CheckPid(last_pid)) {
    ORC_ERROR("find previous process(pid: %lu) exist, please stop it first.", last_pid);
    return false;
  }

  if (!InitOrcConfig(config_file_)) {
    ORC_ERROR("Init orc config fail.");
    return false;
  }

  // Setup log perameters.
  uint32_t max_log_length;
  std::string log_level;
  GetOrcConfig(Options::OrcLogMaxLength, &max_log_length);
  GetOrcConfig(Options::OrcLogLevel, &log_level);

  SetLogMaxLength(max_log_length);
  SetLogLevel(log_level);

  // Daemonize
  bool daemon = true;;
  GetOrcConfig(Options::OrcDaemon, &daemon);
  if (daemon && !Daemon()) {
    ORC_ERROR("daemonize the process fail.");
    return false;
  }

  signal(SIGUSR1, AppSignalHandler);

  auto pid_file_guard = MakeScopeGuard(
      [this](){
        SaveAppPid(getpid());
      },

      [this](){
        std::string path = config_file_.substr(0, config_file_.find_last_of('/'));
        path += "/.pid";
        remove(path.c_str());
      });

  if (!StartServers()) {
    ORC_ERROR("StartServers fail.");
    return false;
  }

  SuperviseServers();
  return true;
}

bool Application::Stop() {
  uint64_t last_pid = GetAppPid();
  // if ((last_pid == 0) || (kill(last_pid, 0) != 0)) {
  if (!CheckPid(last_pid)) {
    ORC_ERROR("not found previous process.");
    return false;
  }

  if (kill(last_pid, SIGUSR1) != 0) {
    ORC_ERROR("kill %lu SIGUSR1 fail.", last_pid);
    return false;
  }

  auto wait_times = 60;
  while ((kill(last_pid, 0) == 0) && (wait_times-- > 0)) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  if (kill(last_pid, 0) == 0) {
    ORC_ERROR("Stop process fail. process: %lu.", last_pid);
    return false;
  }

  ORC_INFO("Stop process success.");
  return true;
}

bool Application::Run(int argc, char** argv) {
  if (!ParseCmdArgs(argc, argv)) {
    return false;
  }

  switch (command_) {
    case Command::Start: return Start();
    case Command::Stop: return Stop();
    default: ORC_ERROR("unknown commnad");
  }
  return false;
}

bool Application::ParseCmdArgs(int argc, char** argv) {
  int ch;
  while ((ch=getopt(argc, argv, "c:h")) != -1) {
    switch (ch) {
      case 'c':
        config_file_ = optarg;
        break;
      case 'h':
      default:
        Usage(argv[0]);
        return false;
    }
  }

  if (config_file_.empty()) {
    ORC_ERROR("\'-c\' option must be set");
    Usage(argv[0]);
  }

  if (optind >= argc) {
    Usage(argv[0]);
    return false;
  }

  if (strcmp(argv[optind], "start") == 0) {
    command_ = Command::Start;
    return true;
  } else if (strcmp(argv[optind], "stop") == 0) {
    command_ = Command::Stop;
    return true;
  } else {
    Usage(argv[0]);
    return false;
  }
}


}  // namespace orc
