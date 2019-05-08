/*!
 * \file mem_block.cc
 * \brief In memory block
 */
#include "qed/mem_block.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <cstdio>
#include <iostream>
#include <ostream>
#include <fstream>
#include <string>
#include <stdexcept>

#include "common/exception.h"
#include "common/logging.h"

namespace qed {

Memblock::Memblock(const std::string& file)
    : MemoryFile(file), read_only_(false) {}

Memblock::~Memblock() {
  Close();
}

off_t Memblock::Open(off_t size, bool populate) {
  if (address_ != nullptr) {
    THROW("Memblock re-opened before close");
  }
  int fd;
  read_only_ = size == 0;
  if (read_only_) {
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
      LOG(ERROR) << "truncate file " << file_name_
          << " to " << size << " failed:" << strerror(errno);
      goto ERROR_HANDLE;
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
  address_ = malloc(st.st_size);
  ssize_t read_size;
  ssize_t total_read_size;
  if (address_ == nullptr) {
    size_ = 0;
    LOG(ERROR) << "malloc memory failed, file name:" << file_name_;
    goto ERROR_HANDLE;
  }
  size_ = st.st_size;
  total_read_size = 0;
  do {
    read_size = read(fd,
                     reinterpret_cast<char*>(address_) + total_read_size,
                     size_ - total_read_size);
    if (read_size > 0) {
      total_read_size += read_size;
    }
  } while (read_size > 0);
  if (size_ != total_read_size || read_size < 0) {
    LOG(ERROR) << read_size << " read != file size " << size_
        << " error:" << strerror(errno)
        << " file:" << file_name_;
    goto ERROR_HANDLE;
  }
  flock(fd, LOCK_UN);
  close(fd);
  return st.st_size;

ERROR_HANDLE:
  flock(fd, LOCK_UN);
  close(fd);
  return 0;
}

void Memblock::Close() {
  if (address_) {
    std::ofstream b_stream(file_name_,
                           std::fstream::out
                           | std::fstream::binary
                           | std::fstream::trunc);
    if (b_stream) {
      b_stream.write(reinterpret_cast<const char*>(address_), size_);
      if (!b_stream.good()) {
        throw std::runtime_error(file_name_ + " " + strerror(errno));
      }
    } else {
      throw std::runtime_error(file_name_ + " " + strerror(errno));
    }
    address_ = nullptr;
    size_ = 0;
  }
}

}  // namespace qed
