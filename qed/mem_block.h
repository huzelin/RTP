/*!
 * \file mem_block.h
 * \brief in memory block
 */
#ifndef QED_MEM_BLOCK_H_
#define QED_MEM_BLOCK_H_

#include <cstddef>
#include <string>

#include "qed/memory_file.h"

namespace qed {

class Memblock : public MemoryFile{
 public:
  /**
   * @brief Wrapper for mmaped file access
   * @param file name of file
   */
  explicit Memblock(const std::string& file);
  ~Memblock() override;

  /**
   * @brief Open memblock
   * @param size 0 if open in read only mode, do not write back
   *        after close.
   *        if non-zero, the file will be created and set to
   *        requested size
   *        and written when close
   * @param populate if populate, read file content to
   *        memory when opened
   * @return size of allocated memory, 0 if open
   *         failure
   * @throw runtime_error if file is opened twice
   *        before Close was called
   */
  off_t Open(off_t size = 0, bool populate = true) override;

  /**
   * @brief Close mmaped memory and file
   * @throws std::runtime_error if write file failure
   */
  void Close() override;

 protected:
  bool read_only_;
};

}  // namespace qed

#endif  // QED_MEM_BLOCK_H_
