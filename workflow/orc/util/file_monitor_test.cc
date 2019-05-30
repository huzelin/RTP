#include "orc/util/file_monitor.h"

#include <thread>
#include <chrono>

#include "gtest/gtest.h"

namespace orc {

static void file_touch(const std::string& file) {
  std::string command = "touch ";
  command.append(file);
  system(command.c_str());
}

static void file_rm(const std::string& file) {
  std::string command = "rm ";
  command.append(file);
  system(command.c_str());
}

static void file_write(const std::string& file, const std::string& content) {
  std::string command = "echo ";
  command.append(content).append(" > ").append(file);
  system(command.c_str());
}

class Counter {
 public:
  Counter() : count_(0) {}

  void Foo(const std::string& file, FileMonitor::Event event) {
    count_++;
    events_.emplace_back(event);
  }

  int32_t count_;
  std::vector<FileMonitor::Event> events_;
};


class FileMonitorTest : public testing::Test {
 public:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }

  static void TearDownTestCase() {
    FileMonitor::Instance()->Stop();
  }
};

TEST_F(FileMonitorTest, Start_Stop) {
  ASSERT_TRUE(FileMonitor::Instance()->Start());
  ASSERT_FALSE(FileMonitor::Instance()->Start());
  FileMonitor::Instance()->Stop();
  FileMonitor::Instance()->Stop();
}

using std::placeholders::_1;
using std::placeholders::_2;

TEST_F(FileMonitorTest, Observe_Forget) {
  std::string file = "file_monitor.test0";
  file_touch(file);
  Counter counter;
  FileMonitor::Instance()->Start();

  auto cookie = FileMonitor::Instance()->Observe(
      file, std::bind(&Counter::Foo, &counter, _1, _2));
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  file_touch(file);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  ASSERT_EQ(1, counter.count_);
  ASSERT_EQ(FileMonitor::Event::Modify, counter.events_[0]);

  file_rm(file);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  ASSERT_EQ(2, counter.count_);
  ASSERT_EQ(FileMonitor::Event::Delete, counter.events_[1]);

  file_touch(file);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  ASSERT_EQ(4, counter.count_);
  ASSERT_EQ(FileMonitor::Event::Create, counter.events_[2]);
  ASSERT_EQ(FileMonitor::Event::Modify, counter.events_[3]);

  std::string file1 = "file_monitor.test1";
  file_touch(file1);
  Counter counter1;
  auto cookie1 = FileMonitor::Instance()->Observe(
      file1, std::bind(&Counter::Foo, &counter1, _1, _2));
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  file_rm(file1);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  ASSERT_EQ(1, counter1.count_);
  ASSERT_EQ(FileMonitor::Event::Delete, counter1.events_[0]);

  FileMonitor::Instance()->Forget(cookie);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  file_touch(file);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  ASSERT_EQ(4, counter.count_);

  FileMonitor::Instance()->Forget(cookie1);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  file_write(file1, "aaa");
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  ASSERT_EQ(1, counter1.count_);
}

}  // namespace orc
