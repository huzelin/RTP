#ifndef LEADER_HEARTBEAT_MESSAGE_HELPER_H_
#define LEADER_HEARTBEAT_MESSAGE_HELPER_H_

#include "heartbeat_message.pb.h"

namespace leader {

bool operator<(const HeartbeatMessage& left, const HeartbeatMessage& right);

bool operator==(const HeartbeatMessage& left, const HeartbeatMessage& right);

bool operator!=(const HeartbeatMessage& left, const HeartbeatMessage& right);

}

#endif  // LEADER_HEARTBEAT_MESSAGE_HELPER_H_
