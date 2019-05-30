#ifndef ORC_UTIL_STRING_PIECE_H_
#define ORC_UTIL_STRING_PIECE_H_

#include <string.h>
#include <stddef.h>

#include <string>
#include <iterator>
#include <algorithm>

namespace orc {

class StringPiece {
 public:
  // stl std::string member types
  using size_type = size_t;
  using value_type = char;
  using pointer = const value_type*;
  using reference = const value_type&;
  using const_reference = const value_type&;
  using difference_type = ptrdiff_t;
  using const_iterator = const char*;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  static constexpr size_type npos = size_type(-1);

 public:
  StringPiece() : ptr_(nullptr), length_(0) {}
  StringPiece(const value_type* str)  // NOLINT
      : ptr_(str), length_(str == nullptr ? 0 : strlen(str)) {}
  StringPiece(const std::string& str)  // NOLINT
      : ptr_(str.data()), length_(str.size()) {}
  StringPiece(const value_type* str, size_type len) : ptr_(str), length_(len) {}

  // stl std::string member functions
  StringPiece& assign(const char* str, size_t len) {
    ptr_ = str;
    length_ = len;
    return *this;
  }

  const_iterator begin() const { return ptr_; }
  const_iterator end() const { return ptr_ + length_; }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(ptr_ + length_);
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(ptr_);
  }

  const value_type* data() const { return ptr_; }

  size_type size() const { return length_; }
  size_type length() const { return length_; }
  size_type max_size() const { return length_; }
  size_type capacity() const { return length_; }

  bool empty() const { return length_ == 0; }

  void clear() {
    ptr_ = nullptr;
    length_ = 0;
  }

  size_type copy(value_type* buf, size_type n, size_type pos = 0) const {
    size_type ret = std::min(length_ - pos, n);
    memcpy(buf, ptr_+pos, ret * sizeof(value_type));
    return ret;
  }

  value_type operator[](size_type i) const { return ptr_[i]; }

  int compare(const StringPiece& o) const {
    int r = memcmp(ptr_, o.ptr_, ((length_ < o.length_) ? length_ : o.length_));
    if (r == 0) {
      if (length_ < o.length_) r = -1;
      else if (length_ > o.length_) r = 1;
    }
    return r;
  }

  size_type find(value_type c, size_type pos = 0) const;
  size_type find(const StringPiece& s, size_type pos = 0) const;

  size_type find_first_of(value_type c, size_type pos = 0) const {
    return find(c, pos);
  }

  size_type find_first_of(const StringPiece& s, size_type pos = 0) const;

  StringPiece substr(size_type pos, size_type n = StringPiece::npos) const;

  // extend functions.
  void Set(const value_type* data, size_type size) {
    ptr_ = data;
    length_ = size;
  }

  void Set(const value_type* data) {
    ptr_ = data;
    length_ = (data == nullptr ? 0 : strlen(data));
  }

  bool StartWith(const StringPiece& o) const {
    return ((length_ >= o.length_) && (memcmp(ptr_, o.ptr_, o.length_) == 0));
  }

  bool EndWith(const StringPiece& o) const {
    return ((length_ >= o.length_) &&
            (memcmp(ptr_ + length_ - o.length_, o.ptr_, o.length_) == 0));
  }

  void RemovePrefix(size_type n) {
    ptr_ += n;
    length_ -= n;
  }

  void RemoveSuffix(size_type n) {
    length_ -= n;
  }

  std::string ToString() const {
    if (empty()) return std::string();
    return std::string(data(), size());
  }

 private:
  const value_type* ptr_;
  size_type length_;
};

inline bool operator==(const StringPiece& l, const StringPiece& r) {
  if (l.size() != r.size()) return false;
  return memcmp(l.data(), r.data(), l.size()) == 0;
}

inline bool operator!=(const StringPiece& l, const StringPiece& r) {
  return !(l == r);
}

inline bool operator==(const StringPiece& l, const std::string& r) {
  if (l.size() != r.size()) return false;
  return memcmp(l.data(), r.data(), l.size()) == 0;
}

inline bool operator!=(const StringPiece& l, const std::string& r) {
  return !(l == r);
}

inline bool operator==(const std::string& l, const StringPiece& r) {
  if (l.size() != r.size()) return false;
  return memcmp(l.data(), r.data(), l.size()) == 0;
}

inline bool operator!=(const std::string& l, const StringPiece& r) {
  return !(l == r);
}

inline bool operator==(const char* l, const StringPiece& r) {
  if (strlen(l) != r.size()) return false;
  return memcmp(l, r.data(), r.size()) == 0;
}

inline bool operator!=(const char* l, const StringPiece& r) {
  return !(l == r);
}

inline bool operator==(const StringPiece& l, const char* r) {
  if (l.size() != strlen(r)) return false;
  return memcmp(l.data(), r, l.size()) == 0;
}

inline bool operator!=(const StringPiece& l, const char* r) {
  return !(l == r);
}

inline bool operator<(const StringPiece& l, const StringPiece& r) {
  auto ret = memcmp(l.data(), r.data(), (l.size() < r.size() ? l.size() : r.size()));
  return ((ret < 0) || ((ret == 0) && (l.size() < r.size())));
}

inline bool operator>(const StringPiece& l, const StringPiece& r) {
  return r < l;
}

inline bool operator<=(const StringPiece& l, const StringPiece& r) {
  return !(l > r);
}

inline bool operator>=(const StringPiece& l, const StringPiece& r) {
  return !(l < r);
}

}  // namespace orc

#endif  // ORC_UTIL_STRING_PIECE_H_
