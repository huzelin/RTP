#include "orc/util/file_monitor.h"

#include <sys/inotify.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <limits.h>
#include <stdlib.h>

#include <list>

#include "orc/util/log.h"
#include "orc/util/str_util.h"

namespace orc {

struct FileMonitor::Cookie {
  std::string file;
  FileMonitor::Action action;
};

struct FileMonitor::Command {
  enum class Type { Observe, Forget };

  Type type;
  FileMonitor::Cookie* cookie;
};

struct FileMonitor::MonitorEntry {
  int wd;
  std::string dir;
  std::map<std::string, std::list<FileMonitor::Cookie*>> entries;
};

FileMonitor::FileMonitor()
    : worker_stop_(false),
      status_(FileMonitor::Status::Stop) {}

FileMonitor* FileMonitor::Instance() {
  static FileMonitor file_monitor;
  return &file_monitor;
}

bool FileMonitor::Start() {
  std::lock_guard<std::mutex> lock(status_mutex_);
  if (status_ != Status::Stop) {
    ORC_WARN("FileMonitor is starting or started.");
    return false;
  }

  inotify_fd_ = inotify_init1(IN_NONBLOCK);
  if (inotify_fd_ == -1) {
    ORC_ERROR("Call inotify_init1 fail. errno: %d.", errno);
    return false;
  }

  epoll_fd_ = epoll_create(1);
  if (epoll_fd_ == -1) {
    ORC_ERROR("Call epool_create fail. errno: %d.", errno);
    return false;
  }

  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.ptr = this;
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, inotify_fd_, &ev) != 0) {
    ORC_ERROR("Call epoll_ctl fail. errno: %d.", errno);
    return false;
  }

  worker_stop_ = false;
  worker_ = std::thread([this]() { WorkLoop(); });

  status_ = Status::Start;
  ORC_INFO("FileMonitor Start success.");
  return true;
}

void FileMonitor::Stop() {
  std::lock_guard<std::mutex> lock(status_mutex_);
  if (status_ == Status::Stop) {
    ORC_WARN("FileMonitor is stopped.");
    return;
  }
  status_ = Status::Stop;

  worker_stop_ = true;
  worker_.join();

  close(inotify_fd_);
  close(epoll_fd_);

  cmds_.clear();

  for (const auto& entry : name_entry_map_) {
    delete entry.second;
  }
  name_entry_map_.clear();
  wd_entry_map_.clear();

  for (const auto& cookie : cookie_bookmark_) {
    delete cookie;
  }
  cookie_bookmark_.clear();
}

FileMonitor::Cookie* FileMonitor::Observe(const std::string& file, const Action& action) {
  if (status_ != Status::Start) {
    ORC_ERROR("FileMonitor isn't Start.");
    return nullptr;
  }

  char path[PATH_MAX];
  if (realpath(file.c_str(), path) == nullptr) {
    ORC_ERROR("Get real path of file: %s fail.", file.c_str());
    return nullptr;
  }

  struct stat st;
  if (stat(path, &st) != 0) {
    ORC_ERROR("Call stat for file: %s fail.", path);
    return nullptr;
  }

  if (!S_ISREG(st.st_mode)) {
    ORC_ERROR("File: %s is not a regular file.", path);
    return nullptr;
  }

  Cookie* cookie = AcquireCookie();
  cookie->file = path;
  cookie->action = action;

  Command cmd{ Command::Type::Observe, cookie };
  std::lock_guard<std::mutex> lock(cmd_mutex_);
  cmds_.emplace_back(cmd);
  return cookie;
}

void FileMonitor::Forget(FileMonitor::Cookie* cookie) {
  if (status_ != Status::Start) {
    ORC_ERROR("FileMonitor isn't Start.");
    return;
  }

  Command cmd{ Command::Type::Forget, cookie };
  std::lock_guard<std::mutex> lock(cmd_mutex_);
  cmds_.emplace_back(cmd);
}

FileMonitor::Cookie* FileMonitor::AcquireCookie() {
  Cookie* cookie = new Cookie();
  std::lock_guard<std::mutex> lock(cookie_mutex_);
  cookie_bookmark_.insert(cookie);
  return cookie;
}

void FileMonitor::ReleaseCookie(FileMonitor::Cookie* cookie) {
  std::lock_guard<std::mutex> lock(cookie_mutex_);
  if (cookie_bookmark_.find(cookie) != cookie_bookmark_.end()) {
    cookie_bookmark_.erase(cookie);
    delete cookie;
  }
}

void FileMonitor::ProcessCommand() {
  if (cmds_.empty()) return ;

  std::vector<Command> cmds;
  {
    std::lock_guard<std::mutex> lock(cmd_mutex_);
    cmds.swap(cmds_);
  }

  for (const auto& cmd : cmds) {
    switch (cmd.type) {
      case Command::Type::Observe:
        ProcessObserve(cmd.cookie);
        break;
      case Command::Type::Forget:
        ProcessForget(cmd.cookie);
        break;
      default: break;
    }
  }
}

void FileMonitor::GetDirAndBasename(
    const std::string& path, std::string* dir, std::string* basename) {
  auto pos = path.find_last_of("/\\");
  dir->assign(path.substr(0, pos));
  basename->assign(path.substr(pos+1));
}

FileMonitor::Event FileMonitor::ToEvent(uint32_t mask) {
  constexpr uint32_t kCreateEvent = IN_CREATE;
  constexpr uint32_t kModifyEvent = IN_ATTRIB | IN_MODIFY | IN_MOVED_TO;
  constexpr uint32_t kDeleteEvent = IN_DELETE | IN_MOVED_FROM;

  if ((mask & kCreateEvent) != 0) return Event::Create;
  if ((mask & kModifyEvent) != 0) return Event::Modify;
  if ((mask & kDeleteEvent) != 0) return Event::Delete;

  return Event::Other;
}

void FileMonitor::ProcessObserve(FileMonitor::Cookie* cookie) {
  std::string dir;
  std::string basename;
  GetDirAndBasename(cookie->file, &dir, &basename);
  auto it = name_entry_map_.find(dir);
  if (it != name_entry_map_.end()) {
    it->second->entries[basename].emplace_back(cookie);
  } else {
    constexpr uint32_t kWatchFlags = IN_CREATE | IN_ATTRIB | IN_MODIFY |
        IN_MOVED_FROM | IN_MOVED_TO |
        IN_DELETE | IN_DELETE_SELF;
    int wd = inotify_add_watch(inotify_fd_, dir.c_str(), kWatchFlags);
    if (wd == -1) {
      int err = errno;
      ORC_ERROR("call inotify_add_watch for file: %s fail. errno: %d.", dir.c_str(), err);
      return;
    }

    auto entry = new MonitorEntry();
    entry->wd = wd;
    entry->dir = dir;
    entry->entries[basename].emplace_back(cookie);
    name_entry_map_[dir] = entry;
    wd_entry_map_[wd] = entry;
  }
}

void FileMonitor::ProcessForget(FileMonitor::Cookie* cookie) {
  std::string dir;
  std::string basename;
  GetDirAndBasename(cookie->file, &dir, &basename);

  auto it = name_entry_map_.find(dir);
  if (it != name_entry_map_.end()) {
    auto entry = it->second;
    auto subit = entry->entries.find(basename);
    if (subit != entry->entries.end()) {
      auto& cookies = subit->second;
      cookies.remove(cookie);
      if (cookies.empty()) {
        entry->entries.erase(subit);
      }
    }

    if (entry->entries.empty()) {
      name_entry_map_.erase(entry->dir);
      wd_entry_map_.erase(entry->wd);

      if (inotify_rm_watch(inotify_fd_, entry->wd) != 0) {
        int err = errno;
        ORC_ERROR("Call inotify_rm_wath fail for file: %s. errno: %d", entry->dir.c_str(), err);
      }
      delete entry;
    }
  }
  ReleaseCookie(cookie);
}

void FileMonitor::ProcessInotify() {
  alignas(alignof(struct inotify_event)) char buf[4096];
  int len = 0;
  io_buf_.clear();

  while ((len = read(inotify_fd_, buf, sizeof buf)) > 0) {
    io_buf_.append(buf, len);
  }

  const struct inotify_event* event;
  for (auto ptr = io_buf_.data(); ptr < io_buf_.data() + io_buf_.size();
       ptr += sizeof (*event) + event->len) {
    event = (const struct inotify_event*)ptr;
    // Event from a file.
    if (event->name[0] != '\0') {
      auto it = wd_entry_map_.find(event->wd);

      if (it != wd_entry_map_.end()) {
        auto entry = it->second;
        auto cookies_it = entry->entries.find(event->name);
        if (cookies_it != entry->entries.end()) {
          Event ev = ToEvent(event->mask);
          for (const auto& cookie : cookies_it->second) {
            cookie->action(cookie->file, ev);
          }
        }
      }
    }
  }
}

void FileMonitor::WorkLoop() {
  while (!worker_stop_) {
    // Process commands
    ProcessCommand();

    // Process events
    // Only one inotify file descriptor in epoll.
    struct epoll_event ev;
    int res = epoll_wait(epoll_fd_, &ev, 1, 10);  // 10ms

    if (res > 0) {
      ProcessInotify();
    }
  }
}

}  // namespace orc
