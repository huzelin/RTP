/*!
 * \file mmap.cc
 * \brief The mmap impls
 */
#include "qed/mmap.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <stdexcept>

#include "common/common_defines.h"
#include "common/logging.h"

#if __linux__
#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,22)
#define _MAP_POPULATE_AVAILABLE
#endif
#endif

#ifndef _MAP_POPULATE_AVAILABLE
#define MAP_POPULATE 0
#endif

#ifndef QED_SUPPORTED_HUGEPAGE_SIZE
/**
 * @brief normally this value wouldn't change and it's kind of tricky
 *        to get reliable huge page size,
 *        so we decided to make it compile time configured
 */
#define QED_SUPPORTED_HUGEPAGE_SIZE 2097152
#endif

namespace qed {

MmapedMemory::MmapedMemory(const std::string& file) : MemoryFile(file) {}

MmapedMemory::~MmapedMemory() {
  Close();
}

off_t MmapedMemory::Open(off_t size, bool populate) {
  if (address_ != nullptr) {
    throw std::runtime_error("MmapedMemory re-opened before close");
  }
  int fd;
  bool read_only = size == 0;
  if (read_only) {
    fd = open(file_name_.c_str(), O_RDONLY);
  } else {
    fd = open(file_name_.c_str(), O_RDWR | O_CREAT, 0666);
  }
  if (fd == -1) {
    LOG(ERROR) << "open file " << file_name_
        << " failed:" << strerror(errno);
    return 0;
  }
  flock(fd, LOCK_EX);
  struct stat st{0};
  if (fstat(fd, &st) != 0) {
    LOG(ERROR) << "get stat of file " << file_name_ << " failed";
    goto ERROR_HANDLE;
  }
  if (size > 0) {
    if (ftruncate(fd, size) != 0) {
      off_t aligned_size = AlignN(size, QED_SUPPORTED_HUGEPAGE_SIZE);
      if (aligned_size == size
          || ftruncate(fd, aligned_size) != 0) {
        LOG(ERROR) << "truncate file " << file_name_
            << " to " << size << " failed:" << strerror(errno);
        goto ERROR_HANDLE;
      }
    }
    // file size may be different to target value var filesystem
    if (fstat(fd, &st) != 0) {
      LOG(ERROR) << "get stat of file " << file_name_ << " failed";
      goto ERROR_HANDLE;
    }
    if (st.st_size < size) {
      LOG(WARNING) << "truncated file size smaller than expect:"
          << st.st_size << " vs " << size;
    }
  }
  address_ = mmap(nullptr,
                  st.st_size,
                  PROT_READ | (size > 0 ? PROT_WRITE : 0),
                  (read_only ? MAP_PRIVATE : MAP_SHARED)
                  | (populate ? MAP_POPULATE : 0),
                  fd,
                  0);
  if (address_ == MAP_FAILED) {
    address_ = nullptr;
    size_ = 0;
    LOG(ERROR) << "mmap file error:"
        << strerror(errno) << ", file name:" << file_name_.c_str();
    goto ERROR_HANDLE;
  }
  size_ = st.st_size;
  flock(fd, LOCK_UN);
  close(fd);
  return st.st_size;

ERROR_HANDLE:
  flock(fd, LOCK_UN);
  close(fd);
  return 0;
}

void MmapedMemory::Close() {
  if (address_) {
    munmap(address_, size_);
    address_ = nullptr;
    size_ = 0;
  }
}

}  // namespace qed
