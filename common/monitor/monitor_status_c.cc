#include "common/monitor/monitor_status_c.h"
#include "common/monitor/monitor_status_impl.h"

int  monitor_status_init(const char *monitor_status_file)
{
    return MONITOR_STATUS_INIT(monitor_status_file) ? 0 : 1;
}

void monitor_status_counter_by(const char *module_name, const char *counter_name, uint64_t inc_count)
{
    MONITOR_STATUS_NORMAL_COUNTER_BY(module_name, counter_name, inc_count);
}

void monitor_status_timer_counter_by(const char *module_name, const char *counter_name, uint64_t total_time, uint64_t total_count)
{
    MONITOR_STATUS_NORMAL_TIMER_BY(module_name, counter_name, total_time, total_count);
}

void monitor_status_binary_counter_by(const char *module_name, const char *counter_name, uint32_t flag)
{
    MONITOR_STATUS_NORMAL_BINARY_BY(module_name, counter_name, flag);
}

