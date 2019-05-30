#include "orc/util/string_piece.h"

#include <climits>
#include <algorithm>

namespace orc {


StringPiece::size_type StringPiece::find(StringPiece::value_type c,
                                         StringPiece::size_type pos) const {
  if (pos >= length_) return npos;

  auto res = std::find(begin() + pos, end(), c);
  return (res != end()) ? static_cast<StringPiece::size_type>(res - begin()) : npos;
}

StringPiece::size_type StringPiece::find(
    const StringPiece& s, StringPiece::size_type pos) const {
  if (pos >= size()) return npos;
  auto res = std::search(begin() + pos, end(), s.begin(), s.end());
  auto xret = static_cast<size_type>(res - begin());
  return ((xret + s.size() <= size()) ? xret : npos);
}

StringPiece::size_type StringPiece::find_first_of(
    const StringPiece& s, StringPiece::size_type pos) const {
  if (size() == 0 || s.size() == 0) return npos;

  if (s.size() == 1) return find(s[0], pos);

  bool lookup[UCHAR_MAX + 1] = {false};
  for (size_type i = 0; i < s.size(); ++i) {
    lookup[static_cast<unsigned char>(s[i])] = true;
  }

  for (size_type i = pos; i < size(); ++i) {
    if (lookup[static_cast<unsigned char>(data()[i])]) return i;
  }
  return npos;
}

StringPiece StringPiece::substr(
    StringPiece::size_type pos, StringPiece::size_type n) const {
  if (pos > size()) pos = size();
  if (n > size() - pos) n = size() - pos;
  return StringPiece(data() + pos, n);
}

}  // namespace orcc
