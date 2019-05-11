/*
 * \file waiter_test.cc
 * \brief The waiter test unit
 */
#include "gtest/gtest.h"

#include <thread>

#include "common/waiter.h"
#include "common/common_defines.h"

namespace common {

void JobRunner(Waiter* waiter) {
  waiter->Notify();
  waiter->Notify();
}

TEST(Waiter, TestAll) {
  Waiter waiter(2);
  std::thread thread1(JobRunner, &waiter);
  waiter.Wait();
  EXPECT_TRUE(true);
  thread1.join();

  waiter.Reset(2);
  std::thread thread2(JobRunner, &waiter);
  waiter.Wait();
  EXPECT_TRUE(true);
  thread2.join();
}

TEST(Waiter, Construct) {
  std::shared_ptr<Waiter> waiter1(new Waiter());
  std::shared_ptr<Waiter> waiter2(nullptr, SharedNoDestroy<Waiter>);
  
  waiter2 = std::move(waiter1);

  //auto mitem = std::shared_ptr<Waiter>(std::move(waiter).get());
  //mitem.get();
  //if (del_p) *del_p = SharedNoDestroy<Waiter>;

  
}

}  // namespace common

