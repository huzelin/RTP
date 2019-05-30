#ifndef ORC_FRAMEWORK_OPTIONS_H_
#define ORC_FRAMEWORK_OPTIONS_H_

#include <string>

namespace orc {

struct Options {
  static std::string OrcDaemon;
  static std::string OrcInFile;
  static std::string OrcOutFile;
  static std::string OrcErrFile;
  static std::string OrcLogMaxLength;
  static std::string OrcLogLevel;

  static std::string SvrName;
  static std::string SvrMaxRestartIntensity;
  static std::string SvrMaxRestartPeriod;
  static std::string SvrIP;
  static std::string SvrPort;
  static std::string SvrIoThreadNum;
  static std::string SvrWorkerNum;
  static std::string SvrWorkerQueueNum;
  static std::string SvrWorkerQueueSize;
  static std::string SvrWorkerPinCpu;
  static std::string SvrServiceDiscovery;
  static std::string SvrMonitorStatusFile;
  static std::string SvrWorkflowPoolMode;
  static std::string SvrWorkflowPoolSize;
  static std::string SvrWorkflowUseTls;
  static std::string SvrWorkflowFile;
  static std::string SvrHandlerFile;
  static std::string SvrSessionFactory;
  static std::string SvrSessionPoolSize;
  static std::string SvrWorkflowRouter;

  static std::string ComEnableList;
  static std::string ComDisableList;
  static std::string ComRpcClientFile;
  static std::string ComDataWorehouseFile;
  static std::string ComHttpClientFile;

  static std::string PluginPath;

  // arpc server config
  static std::string SvrArpcQueueSize;

  // hsf server config
  static std::string SvrHsfConfigHost;
  static std::string SvrHsfAppName;
  static std::string SvrHsfWorkerNum;
  static std::string SvrHsfConnTimeout;
  static std::string SvrHsfResponseTimeout;
  static std::string SvrHsfSvrResponseTimeout;
  static std::string SvrHsfHsfLogEnable;
  static std::string SvrHsfHsfLogLevel;
  static std::string SvrHsfHsfLogPath;
  static std::string SvrHsfHsfLogFile;
  static std::string SvrHsfHsfLogKeepDays;
  static std::string SvrHsfServiceName;
  static std::string SvrHsfServiceVersion;
  static std::string SvrHsfServiceGroup;
  static std::string SvrHsfMethod;
};

}  // namespace orc

#endif  // ORC_FRAMEWORK_OPTIONS_H_
