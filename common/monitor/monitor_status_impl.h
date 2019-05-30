#ifndef COMMON_MONITOR_MONITORSTATUSIMPL_H_
#define COMMON_MONITOR_MONITORSTATUSIMPL_H_

#include <map>
#include <string>
#include <stdint.h>
#include <sys/time.h>

#include "common/common_defines.h"
#include "common/monitor/monitor_status.h"

namespace common {

const uint32_t kTimerMonitorStatusTimeSlice = 500; //500us
const uint32_t kTimerMonitorStatusTimeSlot = 10;
const uint32_t kTimerMonitorStatus1000ms = 1000000;
const uint32_t kTimerMonitorStatus300ms = 300000;
const uint32_t kTimerMonitorStatus100ms = 100000;
const uint32_t kTimerMonitorStatus50ms = 50000;
const uint32_t kTimerMonitorStatus5ms = 5000;
//
// 计时器类型的监控统计状态数据
//
struct TimerMonitorStatus {
    uint64_t total_count; //总次数
    uint64_t total_time; //总时间
    uint64_t time_slice_count[kTimerMonitorStatusTimeSlot]; //0.5ms、1.0ms、1.5ms ... 5.0ms
    uint64_t gt5ms;
    uint64_t gt50ms;
    uint64_t gt100ms;
    uint64_t gt300ms;
    uint64_t gt1000ms;
};

//
// 二值类型的监控统计状态数据
//
struct BinaryMonitorStatus {
    uint32_t flag;
    uint32_t false_count; 
    uint32_t true_count; 
};

class MonitorStatus {
public:
    virtual bool Init(const std::string &monitor_stats_file) = 0;
    static MonitorStatus *Instance();
    // for test only
    static void SetGlobalInstance(MonitorStatus* instance);
    virtual volatile uint64_t *GetCounter(const char * module_name, const char * counter_key) = 0;
    virtual volatile TimerMonitorStatus *GetTimerCounter(const char * module_name, const char * timer_key) = 0;
    virtual volatile BinaryMonitorStatus *GetBinaryCounter(const char * module_name, const char * timer_key) = 0;
    virtual void GetAllCounters(std::map<std::string, uint64_t> *all_counters) = 0;
    virtual void GetAllTimerCounters(std::map<std::string, TimerMonitorStatus> *all_timer_counter) = 0;
    virtual void GetAllBinaryCounters(std::map<std::string, BinaryMonitorStatus> *all_binary_counter) = 0;
protected:
    virtual ~MonitorStatus() {}
    MonitorStatus() {}
private:
    static MonitorStatus * instance_;
};

struct TimerCounterUpdater{
    inline TimerCounterUpdater(volatile TimerMonitorStatus *timer_monitor_stats, uint32_t total_time, uint32_t total_count){
        if (timer_monitor_stats == NULL) {
            return;
        }
        uint32_t slot = total_time/ kTimerMonitorStatusTimeSlice;
        if (slot >= kTimerMonitorStatusTimeSlot) {
            slot = kTimerMonitorStatusTimeSlot - 1;
        }
        __sync_fetch_and_add(&timer_monitor_stats->total_count, total_count);
        __sync_fetch_and_add(&timer_monitor_stats->total_time, total_time);
        __sync_fetch_and_add(&(timer_monitor_stats->time_slice_count[slot]), total_count);
        if (slot == kTimerMonitorStatusTimeSlot - 1) {
          if (total_time > kTimerMonitorStatus1000ms) {
            __sync_fetch_and_add(&timer_monitor_stats->gt1000ms, total_count);
          } else if (total_time > kTimerMonitorStatus300ms) {
            __sync_fetch_and_add(&timer_monitor_stats->gt300ms, total_count);
          } else if (total_time > kTimerMonitorStatus100ms) {
            __sync_fetch_and_add(&timer_monitor_stats->gt100ms, total_count);
          } else if (total_time > kTimerMonitorStatus50ms) {
            __sync_fetch_and_add(&timer_monitor_stats->gt50ms, total_count);
          } else if (total_time > kTimerMonitorStatus5ms) {
            __sync_fetch_and_add(&timer_monitor_stats->gt5ms, total_count);
          }
        }
    }
};

struct BinaryCounterUpdater{
    inline BinaryCounterUpdater(volatile BinaryMonitorStatus *binary_monitor_status, uint32_t flag){
        if(binary_monitor_status == NULL){
            return;
        }
        if(flag == 0){
            binary_monitor_status->flag = 0;
            __sync_fetch_and_add(&binary_monitor_status->false_count, 1);
        }else {
            binary_monitor_status->flag = 1;
            __sync_fetch_and_add(&binary_monitor_status->true_count, 1);
        }
    }
};

struct MonitorStatusTimer {
    MonitorStatusTimer(volatile TimerMonitorStatus *timer_monitor_stats):
        start_(0), timer_monitor_stats_(timer_monitor_stats) {
        start_ = GetTime();
    }
    ~MonitorStatusTimer() {
        uint64_t end = GetTime();
        uint64_t len = end - start_;
        ::common::TimerCounterUpdater(timer_monitor_stats_, len, 1);
    }
    uint64_t GetTime() { //usec
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000000 + tv.tv_usec;
    }

private:
    uint64_t start_;
    volatile TimerMonitorStatus *timer_monitor_stats_;
};

}//end namespace common

// __builtin_expect() required  GCC (version >= 2.96）
#ifndef likely
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 9)
    #define likely(expr)       __builtin_expect(!!(expr), 1)
    #define unlikely(expr)     __builtin_expect(!!(expr), 0)

#else
    #define likely(expr)  (expr)
    #define unlikely(expr)  (expr)

#endif
#endif

#define MONITOR_STATUS_INIT(monitor_stats_file)\
    ::common::MonitorStatus::Instance()->Init(monitor_stats_file)

#define MONITOR_STATUS_COUNTER_BY(module_name, counter_name, inc_count) \
    ({\
       static volatile uint64_t *monitor_counter_ptr = NULL;\
       if(unlikely(monitor_counter_ptr == NULL)){\
           monitor_counter_ptr = ::common::MonitorStatus::Instance()->GetCounter((module_name), (counter_name));\
       }\
       __sync_fetch_and_add(monitor_counter_ptr, inc_count);\
    })

#define MONITOR_STATUS_COUNTER(module_name, counter_name) MONITOR_STATUS_COUNTER_BY(module_name, counter_name, 1)

#define MONITOR_STATUS_NORMAL_COUNTER_BY(module_name, counter_name, inc_count)\
    ({\
       volatile uint64_t *monitor_counter_ptr = ::common::MonitorStatus::Instance()->GetCounter((module_name), (counter_name));\
       if(monitor_counter_ptr) {\
            __sync_fetch_and_add(monitor_counter_ptr, inc_count);\
       }\
    })

#define MONITOR_STATUS_NORMAL_COUNTER(module_name, counter_name) \
     MONITOR_STATUS_NORMAL_COUNTER_BY(module_name, counter_name, 1)

#define MONITOR_STATUS_GET_TIMER_PTR(module_name, timer_name)\
    ({\
          static volatile ::common::TimerMonitorStatus *monitor_timer_counter_ptr = NULL;\
          if(unlikely(monitor_timer_counter_ptr == NULL)){\
                monitor_timer_counter_ptr = ::common::MonitorStatus::Instance()->GetTimerCounter((module_name), (timer_name));\
          }\
          monitor_timer_counter_ptr;\
     })

#define MONITOR_STATUS_NORMAL_GET_TIMER_PTR(module_name, timer_name)\
    ({\
          volatile ::common::TimerMonitorStatus *monitor_timer_counter_ptr = \
                ::common::MonitorStatus::Instance()->GetTimerCounter((module_name), (timer_name));\
          monitor_timer_counter_ptr;\
     })

#define MONITOR_STATUS_TIMER(module_name, timer_name)\
        ::common::MonitorStatusTimer CONCATENATE(monitor_status_timer, __LINE__ )(MONITOR_STATUS_GET_TIMER_PTR((module_name), (timer_name)))

#define MONITOR_STATUS_NORMAL_TIMER(module_name, timer_name)\
        ::common::MonitorStatusTimer CONCATENATE(monitor_status_timer, __LINE__ )(MONITOR_STATUS_NORMAL_GET_TIMER_PTR((module_name), (timer_name)))

#define MONITOR_STATUS_NORMAL_TIMER_BY(module_name, timer_name, total_time, total_count)\
        ::common::TimerCounterUpdater( MONITOR_STATUS_NORMAL_GET_TIMER_PTR((module_name), (timer_name)), total_time, total_count)

#define MONITOR_STATUS_NORMAL_GET_BINARY_COUNTER_PTR(module_name, timer_name)\
    ({\
          volatile ::common::BinaryMonitorStatus *monitor_binary_counter_ptr = \
                ::common::MonitorStatus::Instance()->GetBinaryCounter((module_name), (timer_name));\
          monitor_binary_counter_ptr;\
     })

#define MONITOR_STATUS_NORMAL_BINARY_BY(module_name, item_name, flag)\
        ::common::BinaryCounterUpdater( MONITOR_STATUS_NORMAL_GET_BINARY_COUNTER_PTR((module_name), (item_name)), flag)

#endif /* COM_MONITOR_MONITORSTATUSIMPL_H_ */
