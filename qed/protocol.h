/*!
 * \file protocol.h
 * \brief The protocol in QED
 */
#ifndef QED_PROTOCOL_H_
#define QED_PROTOCOL_H_

namespace qed {

struct RawBucket {
  RawBucket() : key_info_field(0), size_field(0), payload(0) {}
  uint64_t key_info_field;
  uint8_t size_field;
  // NOTE: access to bit field need gcc -O2 to get acceptable
  // performance
  uint64_t payload : 56;
  // end of member variables
};

}  // namespace qed

#endif  // QED_PROTOCOL_H_
