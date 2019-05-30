#ifndef ORC_UTIL_FILE_MONITOR_H_
#define ORC_UTIL_FILE_MONITOR_H_

#include <sys/inotify.h>

#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

#include "orc/util/macros.h"

namespace orc {

class FileMonitor {
 public:
  ~FileMonitor() = default;
  static FileMonitor* Instance();

  bool Start();
  void Stop();

  enum class Event { Create, Modify, Delete, Other };
  using Action = std::function<void(const std::string&, Event)>;
  struct Cookie;

  Cookie* Observe(const std::string& file, const Action& action);
  void Forget(Cookie* cookie);

 private:
  FileMonitor();

  Cookie* AcquireCookie();
  void ReleaseCookie(Cookie* cookie);

  void WorkLoop();
  void ProcessCommand();
  void ProcessObserve(Cookie* cookie);
  void ProcessForget(Cookie* cookie);

  void ProcessInotify();

  void GetDirAndBasename(const std::string& path, std::string* dir, std::string* basename);
  Event ToEvent(uint32_t mask);

 private:
  int inotify_fd_;
  int epoll_fd_;

  struct Command;
  std::mutex cmd_mutex_;
  std::vector<Command> cmds_;

  std::thread worker_;
  std::atomic<bool> worker_stop_;

  std::mutex cookie_mutex_;
  std::set<Cookie*> cookie_bookmark_;

  struct MonitorEntry;
  std::map<int, MonitorEntry*> wd_entry_map_;
  std::map<std::string, MonitorEntry*> name_entry_map_;
  std::string io_buf_;

  enum class Status {Start, Stop};
  std::mutex status_mutex_;
  Status status_;

  ORC_DISALLOW_COPY_AND_ASSIGN(FileMonitor);
};

}  // namespaece orc

#endif  // ORC_UTIL_FILE_MONITOR_H_
