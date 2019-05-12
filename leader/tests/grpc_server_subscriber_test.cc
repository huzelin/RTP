/*
 * \file grpc_server_subscriber_test.cc
 * \brief The grpc server subscriber test unit
 */
#include "gtest/gtest.h"

#include <thread>

#include "leader/grpc/grpc_server_subscriber.h"

namespace leader {

TEST(GrpcServerSubscriber, Start) {
  GrpcServerSubscriber grpc_server_subscriber;
  grpc_server_subscriber.Init("10.45.148.167:2181");
  grpc_server_subscriber.Start();
  grpc_server_subscriber.AddPath("/model1");
  while (true) {
    auto channel = grpc_server_subscriber.GetChannel("/model1");
    LOG(INFO) << "channel=" << channel;
    usleep(1000);
  }
}

}  // namespace leader

