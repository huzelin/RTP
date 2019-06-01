/*
 * \file brpc_server_register_test.cc
 * \brief The brpc server register test unit
 */
#include "gtest/gtest.h"

#include <thread>

#include "leader/brpc/brpc_server_register.h"

namespace leader {

TEST(BrpcServerRegister, Start) {
  BrpcServerRegister brpc_server_register;
  brpc_server_register.Init("10.45.148.167:2181", 8197);
  brpc_server_register.Start();
  brpc_server_register.AddPath("/model1");
  while (true) {
    sleep(1);
  }
}

}  // namespace leader

