/*
 * \file brpc_server_subscriber_test.cc
 * \brief The brpc server subscriber test unit
 */
#include "gtest/gtest.h"

#include <thread>

#include "leader/brpc/brpc_server_subscriber.h"

namespace leader {

TEST(BrpcServerSubscriber, Start) {
  BrpcServerSubscriber brpc_server_subscriber;
  brpc_server_subscriber.Init("101.201.148.9:2181");
  brpc_server_subscriber.Start();
  brpc_server_subscriber.AddPath("/model1");
  while (true) {
    auto channel = brpc_server_subscriber.GetChannel("/model1");
    LOG(INFO) << "channel=" << channel;
    sleep(1);
  }
}

}  // namespace leader

