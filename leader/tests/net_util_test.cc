//
// Copyright (c) 2017 Alibaba Inc. All rights reserved.
//
// Author: xianteng.wxt
//
#include <string>
#include <vector>
#include <regex>
#include "leader/net_util.h"
#define GTEST_HAS_TR1_TUPLE 0
#define GTEST_USE_OWN_TR1_TUPLE 0
#include "gtest/gtest.h"

using namespace leader;

TEST(NetUtilTest, GetHostNameTest) {
  std::string hostName;
  EXPECT_TRUE(NetUtil::GetHostName(hostName));
  EXPECT_GT(hostName.length(), 3);
}

TEST(NetUtilTest, GetIPTest) {
  std::vector<std::string> ips;
  EXPECT_TRUE(NetUtil::GetIP(ips));
  EXPECT_GT(ips.size(), 0);
}

TEST(NetUtilTest, ValidatePortTest) {
  int port = 0;
  EXPECT_FALSE(NetUtil::ValidatePort("aab123", port));
  EXPECT_FALSE(NetUtil::ValidatePort("65536", port));
  EXPECT_TRUE(NetUtil::ValidatePort("80", port));
  EXPECT_EQ(80, port);
}
