/*!
 * \file mmap_file.h
 * \brief The mmap
 */
#ifndef QED_MMAP_H_
#define QED_MMAP_H_

#include <cstddef>
#include <string>

#include "qed/memory_file.h"

namespace qed {

class MmapedMemory : public MemoryFile {
 public:
  /**
   * @brief Wrapper for mmaped file access
   * @param file name of file
   */
  explicit MmapedMemory(const std::string& file);
  ~MmapedMemory() override;

  /**
   * @brief Open file for mapping
   * @param size 0 if open in read only mode(MAP_PRIVATE). if non-zero,
   *        the file will be created and set to requested size
   *        and mapped in read write mode(MAP_SHARED)
   * @param populate only take effect when mapped in read write mode(MAP_SHARED)
   * @return size of mmaped memory, 0 if open failure
   * @throw runtime_error if file is opened twice before Close was called
   */
  off_t Open(off_t size = 0, bool populate = true) override;

  /**
   * @brief Close mmaped memory and file
   */
  void Close() override;
};

}  // namespace qed

#endif  // QED_MMAP_H_
