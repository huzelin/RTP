#include "orc/framework/orc_mgr.h"

#include <stdio.h>

#include "common/monitor/monitor_status.h"
#include "orc/framework/configure.h"
#include "orc/com/component_mgr.h"
#include "orc/util/file_monitor.h"

namespace orc {

OrcMgr* OrcMgr::Instance() {
  static OrcMgr mgr;
  return &mgr;
}

bool OrcMgr::InitMonitorStatus(const YAML::Node& config) {
  std::string file;
  ORC_CONFIG_OR_FAIL(config, Options::SvrMonitorStatusFile, file);
  remove(file.c_str());

  MONITOR_STATUS_INIT(file.c_str());
  ORC_INFO("MonitorStatus Init success.");
  return true;
}

bool OrcMgr::InitFileMonitor(const YAML::Node& config) {
  return FileMonitor::Instance()->Start();
}

bool OrcMgr::InitComponents(const YAML::Node& config) {
  return ComponentMgr::Instance()->Setup(config);
}

bool OrcMgr::InitPlugins(const YAML::Node& config) {
  std::string path;
  if (!GetOrcConfig(config, Options::PluginPath, &path)) {
    ORC_INFO("Option: %s is not set, disable plugin.",
             Options::PluginPath.c_str());
    return true;
  }

  plugin_mgr_.reset(new PluginMgr());
  return plugin_mgr_->Init(path);
}

bool OrcMgr::InitWorkflow(const YAML::Node& config) {
  workflow_.reset(new Workflow());
  return workflow_->Setup(config);
}

bool OrcMgr::Setup(const YAML::Node& config) {
  if (!InitMonitorStatus(config) ||
      !InitFileMonitor(config) ||
      !InitComponents(config) ||
      !InitPlugins(config) ||
      !InitWorkflow(config)) {
    ORC_ERROR("OrcMgr Setup fail.");
    return false;
  }

  ORC_INFO("OrcMgr Setup success.");
  return true;
}

void OrcMgr::Stop() {
  if (workflow_) workflow_->Stop();

  ComponentMgr::Instance()->Release();
  FileMonitor::Instance()->Stop();
}

void OrcMgr::Run(ServiceClosure* closure) {
  workflow_->Run(closure);
}

}  // namespace orc
