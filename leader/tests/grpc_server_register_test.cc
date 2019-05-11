/*
 * \file grpc_server_register_test.cc
 * \brief The grpc server register test unit
 */
#include "gtest/gtest.h"

#include <thread>

#include "leader/grpc/grpc_server_register.h"

namespace leader {

TEST(GrpcServerRegister, Start) {
  GrpcServerRegister grpc_server_register;
  grpc_server_register.Init("10.45.148.167:2181", 8197);
  grpc_server_register.Start();
  grpc_server_register.AddPath("/model1");
  while (true) {
    sleep(1);
  }
}

}  // namespace leader

