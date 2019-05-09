/*!
 * \file heartbeat_message_helper.cc
 * \brief The heartbeat message helper
 */
#include "leader/heartbeat_message_helper.h"

namespace leader {

bool operator<(const HeartbeatMessage& left, const HeartbeatMessage& right) {
  if (left.ip() != right.ip()) {
    return left.ip() < right.ip();
  }
  return (left.port()) < (right.port());
}

bool operator==(const HeartbeatMessage& left, const HeartbeatMessage& right) {
  return ((left.ip() == right.ip()) && (left.port() == right.port()));
}

bool operator!=(const HeartbeatMessage& left, const HeartbeatMessage& right) {
  return (!(left == right));
}

}  // namespace leader
