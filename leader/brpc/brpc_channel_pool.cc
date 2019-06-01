/*!
 * \file brpc_channel_pool.cc
 * \brief The BRPC channel pool
 */
#include "leader/brpc/brpc_channel_pool.h"

#include <unistd.h>
#include <set>
#include <map>
#include <string>
#include <memory>
#include <utility>

#include "common/logging.h"
#include "leader/brpc/brpc_call.h"

using std::map;
using std::string;
using std::set;

namespace leader {

BrpcChannelPool::BrpcChannelPool(uint32_t comm_timeout,
                                 uint32_t build_conn_timeout)
    : timeout_(comm_timeout),
      build_conn_timeout_(build_conn_timeout) {
}

BrpcChannelPool::~BrpcChannelPool() {
  common::ScopedWriteLock writeLock(channel_lock_);
  channel_group_cache_.clear();
  for (auto& pair : channel_pool_) {
    delete pair.second;
    pair.second = nullptr;
  }
}

bool BrpcChannelPool::Init(int32_t work_thread_num,
                           int32_t io_thread_num,
                           uint32_t channel_count) {
  if (channel_count == 0) {
    LOG(ERROR) << "channel count can't be 0";
    return false;
  }
  channel_count_ = channel_count;
  if (io_thread_num <= 0) {
    io_thread_num = GetLogicCpuNum();  // cpu_num as default
  }
  if (work_thread_num <= 0) {
    work_thread_num = GetLogicCpuNum();  // cpu_num as default
  }

  return true;
}

BRPCChannel* BrpcChannelPool::GetChannel(const string& spec) {
  BrpcChannelGroup* channelGroup = nullptr;
  channel_lock_.rdlock();
  auto iter = channel_pool_.find(spec);
  if (iter != channel_pool_.end()) {
    channelGroup = iter->second;
    channel_lock_.unlock();
  } else {
    channel_lock_.unlock();
    auto newChannelGroup = new BrpcChannelGroup(channel_count_, spec, this);
    channel_lock_.wrlock();
    iter = channel_pool_.find(spec);
    if (iter != channel_pool_.end()) {
      channelGroup = iter->second;
      channel_lock_.unlock();
      delete newChannelGroup;
    } else {
      channel_pool_.insert(std::make_pair(spec, newChannelGroup));
      channel_lock_.unlock();
      channelGroup = newChannelGroup;
    }
  }
  if (channelGroup == nullptr) {
    LOG(ERROR) << "Unexpected error, null channel group found";
    return nullptr;
  }
  if (!channelGroup->IsFull()) {
    OpenNewChannel(spec, channelGroup);
  }
  return channelGroup->PickChannel();
}

void BrpcChannelPool::OpenNewChannel(const string& spec,
                                     BrpcChannelGroup* channelGroup) {
  // try take a slot by add null channel
  auto slot = channelGroup->AddChannel();
  if (slot == nullptr) {
    // all slots are taken
    LOG(WARNING) << "channel group[" << spec << "] is full now.";
    return;
  }

  //brpc::ChannelArguments channel_arguments;
  auto channel = new brpc::Channel();
  if (channel == nullptr) {
    LOG(ERROR) << "Open channel failed, spec[" << spec << "].";
    return;
  }
  brpc::ChannelOptions options;
  options.connect_timeout_ms = build_conn_timeout_;
  options.timeout_ms = timeout_;
  options.max_retry = 3;
  options.protocol = "baidu_std";
  options.connection_type = "single";
  if (channel->Init(spec.c_str(), "", &options) != 0) {
    LOG(ERROR) << "Failed to initialize channel";
    return;
  }
  
  channel_lock_.wrlock();
  channel_group_cache_[channel] = channelGroup;
  channel_lock_.unlock();

  (*slot).reset(channel);

  // do Ping now, we don't care about result.
  // just use this to trigger MoveGood/Bad then set channel state.
  BrpcPingCall::Ping(this, channel, spec);

  DLOG(INFO) << "Open channel successfully spec:[" << spec << "] channel:" << channel;
}

uint32_t BrpcChannelPool::GetLogicCpuNum() {
  return sysconf(_SC_NPROCESSORS_ONLN);
}

BrpcChannelGroup* BrpcChannelPool::FindChannelGroup(const std::string& spec,
                                                    BRPCChannel* channel) {
  BrpcChannelGroup* channelGroup = nullptr;
  common::ScopedReadLock guard(channel_lock_);
  auto iter = channel_group_cache_.find(channel);
  CHECK(iter != channel_group_cache_.end()) << "channel is not found in channel_cache.";
  channelGroup = iter->second;
  
  if (channelGroup == nullptr) {
    LOG(ERROR) << "Find channel group " << spec << " failed";
  }
  return channelGroup;
}

void BrpcChannelPool::MoveFromGood2Bad(const std::string& spec,
                                       BRPCChannel* channel) {
  auto channelGroup = FindChannelGroup(spec, channel);
  if (channelGroup == nullptr) {
    return;
  }
  channelGroup->MoveChannelToBad(channel);
}

void BrpcChannelPool::MoveFromBad2Good(const std::string& spec,
                                       BRPCChannel* channel) {
  auto channelGroup = FindChannelGroup(spec, channel);
  if (channelGroup == nullptr) {
    return;
  }
  channelGroup->MoveChannelToGood(channel);
}

void BrpcChannelPool::CheckAndGetBadNodeSpecs(set<string>& blackList) {
  blackList.clear();
  channel_lock_.rdlock();
  std::vector<std::pair<std::string, BrpcChannelGroup*>> pairs;
  pairs.reserve(channel_pool_.size());
  for (const auto& pair : channel_pool_) {
    pairs.push_back(pair);
  }
  channel_lock_.unlock();
  for (const auto pair : pairs) {
    if (pair.second == nullptr) {
      continue;
    }
    if (!pair.second->CheckChannels(build_conn_timeout_)) {
      blackList.insert(pair.first);
    }
  }
}

}  // namespace leader
