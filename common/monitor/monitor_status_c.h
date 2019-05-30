// All rights reserved.
//
// Date: 2014-11-25
//
#ifndef COMMON_MONITORSTATUS__C_H_
#define COMMON_MONITORSTATUS__C_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int  monitor_status_init(const char *monitor_stats_file);
void monitor_status_counter_by(const char *module_name, const char *counter_name, uint64_t inc_count);
void monitor_status_timer_counter_by(const char *module_name, const char *counter_name, uint64_t total_time, uint64_t total_count);
void monitor_status_binary_counter_by(const char *module_name, const char *counter_name, uint32_t flag);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_MONITORSTATUS__C_H_ */
