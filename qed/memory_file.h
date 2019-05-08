/*!
 * \file memory_file.h
 * \brief The memory file
 */
#ifndef QED_MEMORY_FILE_H_
#define QED_MEMORY_FILE_H_

#include <cstddef>
#include <string>

namespace qed {

class MemoryFile {
 public:
  /**
   * @brief Wrapper for mmaped file access
   * @param file name of file
   */
  explicit MemoryFile(const std::string& file)
      : address_(nullptr), size_(0), file_name_(file) {};
  virtual ~MemoryFile() = default;

  /**
   * @brief copy is disabled
   */
  MemoryFile(const MemoryFile&) = delete;

  /**
   * @brief assign is disabled
   */
  MemoryFile& operator=(const MemoryFile&) = delete;

  /**
   * @brief Open file for mapping
   * @param size 0 if open in read only mode. if non-zero,
   *        the file will be created and set to requested
   *        size
   * @param populate only take effect for mmap
   *        implementation
   * @return size of mmaped memory, 0 if open
   *         failure
   * @throw runtime_error if file is opened
   *        twice before Close was called
   */
  virtual off_t Open(off_t size = 0, bool populate = true) = 0;

  /**
   * @brief Close mmaped memory and file
   */
  virtual void Close() = 0;

  inline void* Get() const {
    return address_;
  }

  inline size_t Size() const {
    return size_;
  }

  const std::string& file_name() const {
    return file_name_;
  }

 protected:
  void* address_;
  size_t size_;
  std::string file_name_;
};

}  // namespace qed

#endif  // QED_MEMORY_FILE_H_
