#include "common/monitor/monitor_status.h"

#include "gtest/gtest.h"

using namespace std;
using namespace common;

const char * monitor_status_file = "/tmp/monitor_status_test.dat";

class MonitorStatusTest : public testing::Test {
public:
    static void SetUpTestCase() {
        remove(monitor_status_file);
        bool ret = MONITOR_STATUS_INIT(monitor_status_file);
    }
    static void TearDownTestCase() {
    }
};

TEST_F(MonitorStatusTest, Init) {
}

TEST_F(MonitorStatusTest, MONITOR_STATUS_COUNTER_BY) {
    MONITOR_STATUS_COUNTER_BY("TEST", "counter_by1", 1);
    MONITOR_STATUS_COUNTER_BY("TEST", "counter_by2", 2);
    std::map<std::string, uint64_t> all_counters;
    MonitorStatus::Instance()->GetAllCounters(&all_counters);
    EXPECT_EQ(all_counters["TEST#counter_by1"], 1u);
    EXPECT_EQ(all_counters["TEST#counter_by2"], 2u);
}

TEST_F(MonitorStatusTest, MONITOR_STATUS_COUNTER) {
    std::map<std::string, uint64_t> all_counters;

    MONITOR_STATUS_COUNTER("TEST", "counter1");
    MonitorStatus::Instance()->GetAllCounters(&all_counters);
    EXPECT_EQ(1u, all_counters["TEST#counter1"]);

    MONITOR_STATUS_COUNTER("TEST", "counter1");
    MonitorStatus::Instance()->GetAllCounters(&all_counters);
    EXPECT_EQ(2u, all_counters["TEST#counter1"]);

}

TEST_F(MonitorStatusTest, MONITOR_STATUS_NORMAL_COUNTER_BY) {
    MONITOR_STATUS_COUNTER_BY("TESTNORMAL", "counter_by1", 1);
    MONITOR_STATUS_COUNTER_BY("TESTNORMAL", "counter_by2", 2);
    std::map<std::string, uint64_t> all_counters;
    MonitorStatus::Instance()->GetAllCounters(&all_counters);
    EXPECT_EQ(all_counters["TESTNORMAL#counter_by1"], 1u);
    EXPECT_EQ(all_counters["TESTNORMAL#counter_by2"], 2u);
}

TEST_F(MonitorStatusTest, MONITOR_STATUS_NORMAL_COUNTER) {
    std::map<std::string, uint64_t> all_counters;

    MONITOR_STATUS_COUNTER("TESTNORMAL", "counter1");
    MonitorStatus::Instance()->GetAllCounters(&all_counters);
    EXPECT_EQ(1u, all_counters["TESTNORMAL#counter1"]);

    MONITOR_STATUS_COUNTER("TESTNORMAL", "counter1");
    MonitorStatus::Instance()->GetAllCounters(&all_counters);
    EXPECT_EQ(2u, all_counters["TESTNORMAL#counter1"]);

}
TEST_F(MonitorStatusTest, MONITOR_STATUS_TIMER) {
    MONITOR_STATUS_TIMER("TEST", "timer1");
    //do some thing here
    sleep(1);
    MONITOR_STATUS_TIMER("TEST", "timer2");
    //do some thing here
    std::map<std::string, TimerMonitorStatus> all_timer_counter;
    MonitorStatus::Instance()->GetAllTimerCounters(&all_timer_counter);
    EXPECT_NE(all_timer_counter.find("TEST#timer1"), all_timer_counter.end());
    EXPECT_NE(all_timer_counter.find("TEST#timer2"), all_timer_counter.end());
    EXPECT_EQ(all_timer_counter.find("TEST#timer3"), all_timer_counter.end());
}

TEST_F(MonitorStatusTest, MONITOR_STATUS_NORMAL_TIMER) {
    MONITOR_STATUS_NORMAL_TIMER("TEST", "normal_timer1");
    //do some thing here
    sleep(1);
    MONITOR_STATUS_NORMAL_TIMER("TEST", "normal_timer2");
    //do some thing here
    std::map<std::string, TimerMonitorStatus> all_timer_counter;
    MonitorStatus::Instance()->GetAllTimerCounters(&all_timer_counter);
    EXPECT_NE(all_timer_counter.find("TEST#normal_timer1"), all_timer_counter.end());
    EXPECT_NE(all_timer_counter.find("TEST#normal_timer2"), all_timer_counter.end());
    EXPECT_EQ(all_timer_counter.find("TEST#normal_timer3"), all_timer_counter.end());
}

TEST_F(MonitorStatusTest, MONITOR_STATUS_NORMAL_TIMER_BY) {
    MONITOR_STATUS_NORMAL_TIMER_BY("TEST", "normal_timer1_by", 1, 1);
    //do some thing here
    sleep(1);
    MONITOR_STATUS_NORMAL_TIMER_BY("TEST", "normal_timer2_by", 1, 1);
    //do some thing here
    std::map<std::string, TimerMonitorStatus> all_timer_counter;
    MonitorStatus::Instance()->GetAllTimerCounters(&all_timer_counter);
    EXPECT_NE(all_timer_counter.find("TEST#normal_timer1_by"), all_timer_counter.end());
    EXPECT_NE(all_timer_counter.find("TEST#normal_timer2_by"), all_timer_counter.end());
    EXPECT_EQ(all_timer_counter.find("TEST#normal_timer3_by"), all_timer_counter.end());
}

TEST_F(MonitorStatusTest, MONITOR_STATUS_NORMAL_BINARY_BY) {
    MONITOR_STATUS_NORMAL_BINARY_BY("TEST", "normal_binary1_by", 1);
    //do some thing here
    sleep(1);
    MONITOR_STATUS_NORMAL_BINARY_BY("TEST", "normal_binary2_by", 0);
    //do some thing here
    std::map<std::string, BinaryMonitorStatus> all_binary_counter;
    MonitorStatus::Instance()->GetAllBinaryCounters(&all_binary_counter);
    EXPECT_NE(all_binary_counter.find("TEST#normal_binary1_by"), all_binary_counter.end());
    EXPECT_NE(all_binary_counter.find("TEST#normal_binary2_by"), all_binary_counter.end());
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
