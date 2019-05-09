/*!
 * \file grpc_channel_pool.cc
 * \brief The GRPC channel pool
 */
#include "leader/grpc/grpc_channel_pool.h"

#include <unistd.h>
#include <set>
#include <map>
#include <string>
#include <memory>
#include <utility>

#include "common/logging.h"

using std::map;
using std::string;
using std::set;

namespace leader {

GrpcChannelPool::GrpcChannelPool(uint32_t comm_timeout,
                                 uint64_t channel_queue_size,
                                 uint32_t build_conn_timeout)
    : timeout_(comm_timeout), queue_size_(channel_queue_size),
      build_conn_timeout_(build_conn_timeout) {
}

GrpcChannelPool::~GrpcChannelPool() {
  common::ScopedWriteLock writeLock(channel_lock_);
  channel_group_cache_.clear();
  for (auto& pair : channel_pool_) {
    delete pair.second;
    pair.second = nullptr;
  }
}

bool GrpcChannelPool::Init(int32_t work_thread_num,
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
#if 0
  // init channel-manager
  channel_manager_ = new EasyGRPCChannelManager(nullptr,
                                               work_thread_num,
                                               io_thread_num);
  if (!channel_manager_->Start()) {
    delete channel_manager_;
    channel_manager_ = nullptr;
    LOG(ERROR) << "Failed to start network IO thread.";
    return false;
  }
  // init conn-callback, callback object is managed by channel manager
  channel_manager_->SetConnCallback(new GrpcChannelPoolConnCallback(this));
#endif
  return true;
}

GRPCChannel* GrpcChannelPool::GetChannel(const string& spec) {
  GrpcChannelGroup* channelGroup = nullptr;
  channel_lock_.rdlock();
  auto iter = channel_pool_.find(spec);
  if (iter != channel_pool_.end()) {
    channelGroup = iter->second;
    channel_lock_.unlock();
  } else {
    channel_lock_.unlock();
    auto newChannelGroup = new GrpcChannelGroup(channel_count_);
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

void GrpcChannelPool::OpenNewChannel(const string& spec,
                                     GrpcChannelGroup* channelGroup) {
  // try take a slot by add null channel
  auto slot = channelGroup->AddChannel(nullptr);
  if (slot == nullptr) {
    // all slots are taken
    LOG(WARNING) << "channel group[" << spec << "] is full now.";
    return;
  }

  int cidx = channelGroup->GetIndexOfSlot(slot);
  if (cidx < 0) {
    LOG(ERROR) << "Get slot index for " << slot
               << " failed, this should not happen";
  }

#if 0
  // create channel but don't do Ping, so OnConnect will not be called
  EasyGRPCChannel* channel =
      channel_manager_->OpenChannel(spec, ERPC_CONN_IDEL_TIME,
                                    0,  // priority, temporarily useless
                                    queue_size_,  // queue_size, temporarily useless
                                    false,
                                    build_conn_timeout_);
  if (channel == nullptr) {
    // only bad spec can cause this,
    // so we don't need to handle the empty slot, just leave it there
    LOG(ERROR) << "Open channel failed, spec[" << spec << "].";
    return;
  }
  // call this interface will set cidx of this channel,
  // which will force libeasy create new connection for this channel
  // the 8 bit offset is to prevent cidx affect io thread picking
  // see implementation of easy_client_dispatch , easy_client_list_find
  // and easy_connection_do_client for detail
  channel->SetSingleAddress((cidx + 1) << 8);
  slot->store(channel);
  channel_lock_.wrlock();
  channel_group_cache_[channel] = channelGroup;
  channel_lock_.unlock();
  // do Ping now, we don't care about the result,
  // just use this to trigger MoveGood/Bad then set channel state
  if (!channel->Ping(build_conn_timeout_)) {
    LOG(ERROR) << "Ping to [" << spec << "] failed";
  }
  channel->SetDefaultTimeout(timeout_);
  DLOG(INFO)
  << "Open channel successfully spec:[" << spec << "] channel:" << channel;
#endif
}

uint32_t GrpcChannelPool::GetLogicCpuNum() {
  return sysconf(_SC_NPROCESSORS_ONLN);
}

GrpcChannelGroup* GrpcChannelPool::FindChannelGroup(const std::string& spec,
                                                    GRPCChannel* channel) {
  GrpcChannelGroup* channelGroup = nullptr;
  common::ScopedReadLock guard(channel_lock_);
  auto iter = channel_group_cache_.find(channel);
  if (iter == channel_group_cache_.end()) {
    auto cgIter = channel_pool_.find(spec);
    if (cgIter == channel_pool_.end()) {
      LOG(ERROR) << "Unknown spec:" << spec;
      return nullptr;
    }
    channelGroup = cgIter->second;
    if (channelGroup->AddChannel(channel) == nullptr) {
      LOG(ERROR) << "Add channel to channel group " << spec << " failed";
      return nullptr;
    }
    channel_group_cache_[channel] = channelGroup;
  } else {
    channelGroup = iter->second;
  }
  if (channelGroup == nullptr) {
    LOG(ERROR) << "Find channel group " << spec << " failed";
  }
  return channelGroup;
}

void GrpcChannelPool::MoveFromGood2Bad(const std::string& spec,
                                       GRPCChannel* channel) {
  auto channelGroup = FindChannelGroup(spec, channel);
  if (channelGroup == nullptr) {
    return;
  }
  channelGroup->MoveChannelToBad(channel);
}

void GrpcChannelPool::MoveFromBad2Good(const std::string& spec,
                                       GRPCChannel* channel) {
  auto channelGroup = FindChannelGroup(spec, channel);
  if (channelGroup == nullptr) {
    return;
  }
  channelGroup->MoveChannelToGood(channel);
}

void GrpcChannelPool::CheckAndGetBadNodeSpecs(set<string>& blackList) {
  blackList.clear();
  channel_lock_.rdlock();
  std::vector<std::pair<std::string, GrpcChannelGroup*>> pairs;
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
