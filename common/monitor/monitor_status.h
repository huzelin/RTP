// All rights reserved.
#ifndef COM_MONITORSTATUS_H_
#define COM_MONITORSTATUS_H_

#ifdef DISABLE_MONITOR_STATUS
    #define MONITOR_STATUS_INIT(monitor_stats_file)
    #define MONITOR_STATUS_COUNTER(module_name, counter_name)
    #define MONITOR_STATUS_NORMAL_COUNTER(module_name, counter_name)
    #define MONITOR_STATUS_COUNTER_BY(module_name, counter_name, inc_count)
    #define MONITOR_STATUS_NORMAL_COUNTER_BY(module_name, counter_name, inc_count)
    #define MONITOR_STATUS_TIMER(module_name, timer_name)
    #define MONITOR_STATUS_NORMAL_TIMER(module_name, timer_name)
    #define MONITOR_STATUS_NORMAL_TIMER_BY(module_name, timer_name, total_time, total_count)
    #define MONITOR_STATUS_NORMAL_BINARY_BY(module_name, item_name, flag)
#else
    #include "monitor_status_impl.h"
#endif

#endif /* COM_MONITORSTATUS_H_ */
