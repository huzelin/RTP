// Description:
//   用于读取监控状态文件,对应于监控文件的parser程序。
//   配合monitor-status.h使用，对com/monitor/monitor-status.h输出结果
//   进行解析。

#include <unistd.h> /* access */
#include <vector>
#include <string>
#include <sys/stat.h> /* stat */

#include "common/monitor/monitor_status_impl.h"
#include <iostream>
#include <boost/format.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace common;
using namespace boost::xpressive;

using boost::lexical_cast;

static uint64_t g_time_interval = 0;
static bool g_show_time_slice = false;
static bool g_show_timestamp = false;
static bool g_show_qnum = false;
static bool g_is_help_show = false;
static string g_monitor_status_file;
static string g_output_type;
static string g_search_str;
static sregex g_search_regex;
static vector<string> attrs;

static const char* help =
    "NAME:                                            \n"
    "    ms_reader - monitor status reader            \n"
    "                                                 \n"
    "SYNOPSIS:                                        \n"
    "    %1% [options]                                \n"
    "                                                 \n"
    "DESCRIPTION                                      \n"
    "    ms_reader used to read monitor status file   \n"
    "                                                 \n"
    "OPTIONS                                          \n"
    "      -h help show this                          \n"
    "      -f monitor status file, must be specific   \n"
    "      -v output format, should be json or text   \n"
    "      -t show time slice status                  \n"
    "      -s search string used to filer counter     \n"
    "      -i set time interval seconds               \n"
    "      -a set topic to monitor                    \n"
    "      -T show timestamp                          \n"
    "      -Q show total count diff between samples   \n"
    "                                                 \n"
    "Example:                                         \n"
    "      %1% -f /tmp/monitor_status_test.dat                               \n"
    "      %1% -f /tmp/monitor_status_test.dat -v test                       \n"
    "      %1% -f /tmp/monitor_status_test.dat -t                            \n"
    "      %1% -f /tmp/monitor_status_test.dat -s \"test|counter\"           \n"
    "      %1% -f /tmp/monitor_status_test.dat -i 1                          \n";

template<typename T>
static int GetCounterNameMaxLen(std::map<std::string, T>& all_counters) {
  uint32_t max_len = 0;
  for (typename std::map<std::string, T>::iterator mit = all_counters.begin();
       mit != all_counters.end(); ++mit) {
    if (mit->first.size() > max_len) {
      max_len = mit->first.size();
    }
  }
  return max_len + 2;
}

static std::string GetTime() {
  time_t tim;
  struct tm* p;
  time(&tim);
  p = gmtime(&tim);
  char tmp[32];
  snprintf(tmp, sizeof(tmp),
           "%02d/%02d/%4d-%02d:%02d:%02d",
           p->tm_mday, p->tm_mon + 1,
           p->tm_year + 1900,
           p->tm_hour + 8,
           p->tm_min, p->tm_sec);
  return std::string(tmp);
}

static void PrintCounterMap(std::map<std::string, uint64_t>& all_counters,
                            std::map<std::string, uint64_t>& old_all_counters) {
  if (all_counters.empty()) {
    return;
  }
  int max_len = GetCounterNameMaxLen(all_counters);
  char buff[64] = "";
  snprintf(buff,
           sizeof(buff) - 1,
           g_show_timestamp ? "%%|-21|%%|-%d|%s" : "%%|-%d|%s",
           max_len,
           "%|-16.0f|%|-16|");
  boost::format counter_format(buff);
  if (g_show_timestamp) {
    counter_format % "time" % "name" % "qps" % "total_count";
  } else {
    counter_format % "name" % "qps" % "total_count";
  }
  cout << counter_format.str() << endl;

  for (std::map<std::string, uint64_t>::iterator mit = all_counters.begin();
       mit != all_counters.end(); ++mit) {
    string counter_name = mit->first;
    uint32_t total_count = mit->second;
    uint32_t old_total_count = 0;
    std::map<std::string, uint64_t>::iterator
        old_mit = old_all_counters.find(counter_name);
    if (old_mit != old_all_counters.end()) {
      old_total_count = old_mit->second;
    }
    double qps = (double) (total_count - old_total_count)
        / (g_time_interval == 0 ? 1 : g_time_interval / 1000000);
    if (g_show_timestamp) {
      counter_format % GetTime().c_str() % counter_name % qps % total_count;
    } else {
      counter_format % counter_name % qps % total_count;
    }
    cout << counter_format.str() << endl;
  }

}

static void PrintBinaryMap(std::map<std::string,
                                    BinaryMonitorStatus>& all_binary_counters,
                           std::map<std::string,
                                    BinaryMonitorStatus>& old_all_binary_counters) {
  if (all_binary_counters.empty()) {
    return;
  }
  int max_len = GetCounterNameMaxLen(all_binary_counters);
  char buff[64] = "";
  snprintf(buff,
           sizeof(buff) - 1,
           g_show_timestamp ? "%%|-21|%%|-%d|%s" : "%%|-%d|%s",
           max_len,
           "%|-5|%|-16.0f|%|-16|%|-16.0f|%|-16|");
  boost::format counter_format(buff);
  if (g_show_timestamp) {
    counter_format % "time" % "name" % "flag" % "true_qps" % "false_qps"
        % "true_count" % "false_count";
  } else {
    counter_format % "name" % "flag" % "true_qps" % "false_qps"
        % "true_count" % "false_count";
  }
  cout << counter_format.str() << endl;

  for (std::map<std::string, BinaryMonitorStatus>::iterator
           mit = all_binary_counters.begin();
       mit != all_binary_counters.end(); ++mit) {
    string counter_name = mit->first;
    BinaryMonitorStatus* status = &mit->second;
    BinaryMonitorStatus* old_status = NULL;
    std::map<std::string, BinaryMonitorStatus>::iterator
        old_mit = old_all_binary_counters.find(counter_name);
    if (old_mit != old_all_binary_counters.end()) {
      old_status = &old_mit->second;
    } else {
      continue;
    }
    double true_qps = (double) (status->true_count - old_status->true_count)
        / (g_time_interval == 0 ? 1 : g_time_interval / 1000000);
    double false_qps = (double) (status->false_count - old_status->false_count)
        / (g_time_interval == 0 ? 1 : g_time_interval / 1000000);
    if (g_show_timestamp) {
      counter_format % GetTime().c_str() % counter_name % status->flag
          % true_qps
          % false_qps % status->true_count % status->false_count;
    } else {
      counter_format % counter_name % status->flag
          % true_qps
          % false_qps % status->true_count % status->false_count;
    }
    cout << counter_format.str() << endl;
  }
}

static TimerMonitorStatus g_null_tm_status;
static void ShowTimeSlice(std::map<std::string,
                                   TimerMonitorStatus>& all_timer_counters,
                          std::map<std::string,
                                   TimerMonitorStatus>& old_all_timer_counters) {
  int max_len = GetCounterNameMaxLen(all_timer_counters);
  char buff[256] = "";
  snprintf(buff,
           sizeof(buff) - 1,
           "%%|-%d|%s",
           max_len,
           "%|-10.2f|"
           "%|-10.2f|"
           "%|-10.2f|"
           "%|-10.2f|"
           "%|-10.2f|"
           "%|-10.2f|"
           "%|-10.2f|"
           "%|-10.2f|"
           "%|-10.2f|"
           "%|-10.2f|"
           "%|-10.2f|"
           "%|-10.2f|"
           "%|-10.2f|"
           "%|-11.2f|"
           "%|-12.2f|");
  boost::format timer_counter_format(buff);
  timer_counter_format % "timer_counter_name"
      % "gt0ms(%)"
      % "gt0.5ms(%)"
      % "gt1.0ms(%)"
      % "gt1.5ms(%)"
      % "gt2.0ms(%)"
      % "gt2.5ms(%)"
      % "gt3.0ms(%)"
      % "gt3.5ms(%)"
      % "gt4.0ms(%)"
      % "gt4.5ms(%)"
      % "gt5ms(%)"
      % "gt50ms(%)"
      % "gt100ms(%)"
      % "gt300ms(%)"
      % "gt1000ms(%)";
  cout << timer_counter_format.str() << endl;
  for (std::map<std::string, TimerMonitorStatus>::iterator
           mit = all_timer_counters.begin();
       mit != all_timer_counters.end(); ++mit) {
    uint64_t slen = sizeof(TimerMonitorStatus) / sizeof(uint64_t) - 2;
    uint64_t* s = reinterpret_cast<uint64_t*>(&mit->second) + 2;
    uint64_t* old_s = reinterpret_cast<uint64_t*>(&g_null_tm_status) + 2;
    if (old_all_timer_counters.find(mit->first)
        != old_all_timer_counters.end()) {
      old_s =
          reinterpret_cast<uint64_t*>(&old_all_timer_counters[mit->first]) + 2;
    }

    uint32_t total_count = 0;
    uint64_t inc_count[slen];
    for (uint32_t i = 0; i < slen; ++i) {
      inc_count[i] = s[i] - old_s[i];
      total_count += inc_count[i];
    }

    timer_counter_format % mit->first;
    for (uint32_t i = 0; i < slen; ++i) {
      timer_counter_format
          % ((double) inc_count[i] / (total_count > 0 ? total_count : 1) * 100);
    }
    cout << timer_counter_format.str() << endl;
  }
}

static void PrintTimerMap(std::map<std::string,
                                   TimerMonitorStatus>& all_timer_counters,
                          std::map<std::string,
                                   TimerMonitorStatus>& old_all_timer_counters) {
  if (all_timer_counters.empty()) {
    return;
  }
  if (g_show_time_slice) {
    ShowTimeSlice(all_timer_counters, old_all_timer_counters);
    return;
  }

  int max_len = GetCounterNameMaxLen(all_timer_counters);
  char buff[64] = "";
  snprintf(buff,
           sizeof(buff) - 1,
           g_show_timestamp ? "%%|-21|%%|-%d|%s" : "%%|-%d|%s",
           max_len,
           g_show_qnum ? "%|-14.2f|%|-14.0f|%|-14|" : "%|-14.2f|%|-14.0f|");
  boost::format counter_format(buff);
  if (g_show_timestamp) {
    counter_format % "time";
  }
  counter_format % "name" % "rt(ms)" % "qps";
  if (g_show_qnum) {
    counter_format % "qnum";
  }
  cout << counter_format.str() << endl;

  for (std::map<std::string, TimerMonitorStatus>::iterator
           mit = all_timer_counters.begin();
       mit != all_timer_counters.end(); ++mit) {
    const string& counter_name = mit->first;
    TimerMonitorStatus* status = &mit->second;
    TimerMonitorStatus* old_status = &g_null_tm_status;
    std::map<std::string, TimerMonitorStatus>::iterator
        old_mit = old_all_timer_counters.find(counter_name);
    if (old_mit != old_all_timer_counters.end()) {
      old_status = &old_mit->second;
    }
    double qps = (double) (status->total_count - old_status->total_count)
        / (g_time_interval == 0 ? 1 : g_time_interval / 1000000);
    uint64_t count_diff = status->total_count - old_status->total_count;
    double rt = (double) (status->total_time - old_status->total_time)
        / (count_diff == 0 ? 1 : count_diff);
    rt /= 1000;
    if (g_show_timestamp) {
      counter_format % GetTime().c_str();
    }
    counter_format % counter_name % rt % qps;
    if (g_show_qnum) {
      counter_format % count_diff;
    }
    cout << counter_format.str() << endl;
  }
}

static void PrintJsonMap(std::map<std::string, uint64_t>& all_counters,
                         std::map<std::string,
                                  TimerMonitorStatus>& all_timer_counter,
                         std::map<std::string,
                                  BinaryMonitorStatus>& all_binary_counter) {
  string result;
  result += "{";
  //timestamp
  struct timeval now;
  gettimeofday(&now, NULL);
  double time_now = now.tv_sec + static_cast<double>(now.tv_usec) / 1000000;
  result += "\"__timestamp\":" + lexical_cast<string>(time_now) + ",";
  //counter
  result += "\"counters\":{";
  for (std::map<std::string, uint64_t>::iterator mit = all_counters.begin();
       mit != all_counters.end(); ++mit) {
    result += "\"" + mit->first + "\":";
    result += lexical_cast<string>(mit->second) + ",";
  }
  if (all_counters.begin() != all_counters.end()) {
    result.erase(result.size() - 1);
  }
  result += "},";
  //timer counter
  result += "\"timer_counters\":{";
  for (std::map<std::string, TimerMonitorStatus>::iterator
           mit = all_timer_counter.begin();
       mit != all_timer_counter.end(); ++mit) {
    TimerMonitorStatus& s = mit->second;
    result += "\"" + mit->first + "\":";
    result += "{";
    result += "\"count\":" + lexical_cast<string>(s.total_count) + ",";
    result += "\"time\":" + lexical_cast<string>(s.total_time);
    result += "},";
  }
  if (all_timer_counter.begin() != all_timer_counter.end()) {
    result.erase(result.size() - 1);
  }
  result += "},";
  //binary counter
  result += "\"binary_counters\":{";
  for (std::map<std::string, BinaryMonitorStatus>::iterator
           mit = all_binary_counter.begin();
       mit != all_binary_counter.end(); ++mit) {
    BinaryMonitorStatus& s = mit->second;
    result += "\"" + mit->first + "\":";
    result += "{";
    result += "\"flag\":" + lexical_cast<string>(s.flag) + ",";
    result += "\"false_count\":" + lexical_cast<string>(s.false_count) + ",";
    result += "\"true_count\":" + lexical_cast<string>(s.true_count);
    result += "},";
  }
  if (all_binary_counter.begin() != all_binary_counter.end()) {
    result.erase(result.size() - 1);
  }
  result += "}}";
  cout << result << endl;
}

static void PrintTextMap(std::map<std::string, uint64_t>& all_counters,
                         std::map<std::string,
                                  TimerMonitorStatus>& all_timer_counter) {
  string result;
  result += lexical_cast<string>(time(0)) + "\t";
  //counter
  for (std::map<std::string, uint64_t>::iterator mit = all_counters.begin();
       mit != all_counters.end(); ++mit) {
    result += mit->first + "=" + lexical_cast<string>(mit->second) + " ";
  }
  for (std::map<std::string, TimerMonitorStatus>::iterator
           mit = all_timer_counter.begin();
       mit != all_timer_counter.end(); ++mit) {
    TimerMonitorStatus& s = mit->second;
    result +=
        mit->first + "#count=" + lexical_cast<string>(s.total_count) + " ";
    result += mit->first + "#time=" + lexical_cast<string>(s.total_time) + " ";
  }
  if (all_timer_counter.begin() != all_timer_counter.end()) {
    result.erase(result.size() - 1);
  }
  cout << result << endl;
}

template<typename T>
static void CounterFilter(std::map<std::string, T>& all_counters,
                          std::map<std::string, T>* result_counters) {
  for (typename std::map<std::string, T>::iterator mit = all_counters.begin();
       mit != all_counters.end(); ++mit) {
    if (g_search_str.empty() || regex_search(mit->first, g_search_regex)) {
      (*result_counters)[mit->first] = mit->second;
    }
  }
}

static int Init(int argc, char* argv[]) {
  while (1) {
    int c = getopt(argc, argv, "Ttha:i:v:s:f:");
    if (c == -1) {
      break;
    }
    switch (c) {
      case 'Q':
        g_show_qnum = true;
        break;
      case 'T':
        g_show_timestamp = true;
        break;
      case 't':
        g_show_time_slice = true;
        break;
      case 'f':
        g_monitor_status_file = optarg;
        break;
      case 'a':
        attrs.push_back(std::string(optarg));
        break;
      case 'i':
        //g_time_interval = atoi(optarg);
      {
        char* arg_end = NULL;
        g_time_interval = strtol(optarg, &arg_end, 10);

        if (arg_end != NULL && arg_end != optarg && *arg_end != '\0') {
          if (*arg_end == 'm' && *(arg_end + 1) == 's') {
            g_time_interval *= 1000;
          } else if (*arg_end != 'u' || *(arg_end + 1) != 's') {
            g_time_interval *= 1000000;
          }
        } else {
          g_time_interval *= 1000000;
        }
      }
        break;
      case 'v':
        g_output_type = optarg;
        break;
      case 's':
        g_search_str = optarg;
        g_search_regex = sregex::compile(g_search_str);
        break;
      case 'h':
      case '?':
        g_is_help_show = true;
        return 1;
    }
  }
  if (g_monitor_status_file.empty()
      || access(g_monitor_status_file.c_str(), R_OK | F_OK)) {
    return 1;
  }
  struct stat info;
  stat(g_monitor_status_file.c_str(), &info);
  if (!S_ISREG(info.st_mode)) {
    return 1;
  }
  return 0;
}

int main(int argc, char* argv[]) {
  if (Init(argc, argv)) {
    cerr << boost::format(help) % argv[0] << endl;
    return 1;

  }
  MonitorStatus::Instance()->Init(g_monitor_status_file);
  std::map<std::string, uint64_t> all_counters;
  std::map<std::string, uint64_t> filtered_all_counters;
  std::map<std::string, uint64_t> old_filtered_all_counters;

  std::map<std::string, TimerMonitorStatus> all_timer_counter;
  std::map<std::string, TimerMonitorStatus> filtered_all_timer_counter;
  std::map<std::string, TimerMonitorStatus> old_filtered_all_timer_counter;

  std::map<std::string, BinaryMonitorStatus> all_binary_counter;
  std::map<std::string, BinaryMonitorStatus> filtered_all_binary_counter;
  std::map<std::string, BinaryMonitorStatus> old_filtered_all_binary_counter;
  if (g_output_type.empty()) {
    MonitorStatus::Instance()->GetAllCounters(&old_filtered_all_counters);
    MonitorStatus::Instance()->GetAllTimerCounters(&old_filtered_all_timer_counter);
    MonitorStatus::Instance()->GetAllBinaryCounters(&old_filtered_all_binary_counter);
    uint64_t usleep_time = g_time_interval > 0 ? g_time_interval : 1000000;
    usleep(usleep_time);
  }
  do {
    MonitorStatus::Instance()->GetAllCounters(&all_counters);
    MonitorStatus::Instance()->GetAllTimerCounters(&all_timer_counter);
    MonitorStatus::Instance()->GetAllBinaryCounters(&all_binary_counter);
    CounterFilter(all_counters, &filtered_all_counters);
    CounterFilter(all_timer_counter, &filtered_all_timer_counter);
    CounterFilter(all_binary_counter, &filtered_all_binary_counter);
    if (g_output_type == "json") {
      PrintJsonMap(filtered_all_counters,
                   filtered_all_timer_counter,
                   filtered_all_binary_counter);
      break;

    } else if (g_output_type == "text") {
      PrintTextMap(filtered_all_counters, filtered_all_timer_counter);
      break;

    } else {
      PrintCounterMap(filtered_all_counters, old_filtered_all_counters);
      PrintTimerMap(filtered_all_timer_counter, old_filtered_all_timer_counter);
      PrintBinaryMap(filtered_all_binary_counter,
                     old_filtered_all_binary_counter);
      old_filtered_all_counters = filtered_all_counters;
      old_filtered_all_timer_counter = filtered_all_timer_counter;
      old_filtered_all_binary_counter = filtered_all_binary_counter;
    }

  } while (g_time_interval && usleep(g_time_interval) == 0);

  return 0;
}
