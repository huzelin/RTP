#include "orc/util/utils.h"

#include <sys/syscall.h>
#include <unistd.h>

#include <stdio.h>
#include <functional>
#include <thread>

namespace orc {

uint32_t RandomMax() {
  return 2147483647;
}

// Get current thread's meta info. Must not pass the return 'ThreadMeta' to
// other thread.
ThreadMeta* GetThreadMeta() {
  static __thread ThreadMeta meta;
  if (!meta.init) {
    meta.rand = (std::hash<std::thread::id>()(std::this_thread::get_id())) % RandomMax();
    meta.tid = static_cast<int>(syscall(SYS_gettid));
    snprintf(meta.tid_str, sizeof(meta.tid_str), "%d", meta.tid);
    meta.init = true;
  }
  return &meta;
}

int tid() {
  ThreadMeta* meta = GetThreadMeta();
  return meta->tid;
}

char* tid_str() {
  ThreadMeta* meta = GetThreadMeta();
  return meta->tid_str;
}

uint32_t Random() {
  ThreadMeta* meta = GetThreadMeta();
  uint32_t r = meta->rand;
  // Come from std::minstd_rand
  meta->rand =  meta->rand * 48271 % RandomMax();
  return r;
}

}  // namespace orc
