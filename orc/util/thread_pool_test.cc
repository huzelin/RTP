#include <atomic>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "gtest/gtest.h"
#include "orc/util/thread_pool.h"
#include "orc/util/utils.h"

namespace orc {

class ThreadPoolTest : public ::testing::Test {
 public:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

TEST_F(ThreadPoolTest, ScheduleRandom) {
  std::atomic<uint32_t> counter(0);
  std::map<uint32_t, std::atomic<uint32_t>> thread_schedule_counts;
  thread_schedule_counts[0].store(0);
  thread_schedule_counts[1].store(0);
  uint32_t fail_counter = 0;
  {
    std::unique_ptr<ThreadPool> pool(new ThreadPool(2, 2));
    for (auto i = 0; i < 10000; ++i) {
      auto task = pool->Schedule(
          [&counter, &thread_schedule_counts]() {
          counter++;
          auto th_meta = GetThreadMeta();
          thread_schedule_counts[th_meta->id]++;
          });
      if (task) fail_counter++;
    }
  }

  ASSERT_EQ(static_cast<uint32_t>(10000), counter.load()+fail_counter);

  uint32_t count0 = thread_schedule_counts[0].load();
  uint32_t count1 = thread_schedule_counts[1].load();
  double ratio = count0 * 1.0 / count1;

  ASSERT_NEAR(1.0, ratio, 0.1);
}

TEST_F(ThreadPoolTest, ScheduleFail) {
  std::unique_ptr<ThreadPool> pool(new ThreadPool(1, 1, 1));
  std::mutex mtx;
  std::condition_variable cv;
  std::atomic<bool> flag{false};
  {
    auto task = pool->Schedule(
        [&mtx, &cv, &flag](){
          flag = true;
          std::unique_lock<std::mutex> lock(mtx);
          cv.wait(lock);
        });
    ASSERT_FALSE(static_cast<bool>(task));
  }
  {
    while (!flag);  // spin
    auto task = pool->Schedule([](){ });
    ASSERT_FALSE(static_cast<bool>(task));
  }
  {
    auto task = pool->Schedule(
        [&cv](){ cv.notify_all(); });
    ASSERT_TRUE(static_cast<bool>(task));
    task();
  }
}

}  // namespace orc
