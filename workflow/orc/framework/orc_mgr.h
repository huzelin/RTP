#ifndef ORC_FRAMEWORK_ORC_MGR_H_
#define ORC_FRAMEWORK_ORC_MGR_H_

#include <memory>

#include "orc/framework/service_closure.h"
#include "orc/framework/workflow.h"
#include "orc/util/thread_pool.h"
#include "orc/plugin/plugin_mgr.h"

#include "yaml-cpp/yaml.h"

namespace orc {

class OrcMgr {
 public:
  ~OrcMgr() = default;
  static OrcMgr* Instance();

  bool Setup(const YAML::Node& config);
  void Stop();

  void Run(ServiceClosure* closure);

  PluginMgr* plugin_mgr() const { return plugin_mgr_.get(); }

 private:
  OrcMgr() = default;

  bool InitMonitorStatus(const YAML::Node& config);
  bool InitFileMonitor(const YAML::Node& config);
  bool InitComponents(const YAML::Node& config);
  bool InitPlugins(const YAML::Node& config);
  bool InitWorkflow(const YAML::Node& config);

 private:
  std::unique_ptr<Workflow> workflow_;
  std::unique_ptr<PluginMgr> plugin_mgr_;
};

}  // namespace orc

#endif  // ORC_FRAMEWORK_ORC_MGR_H_
