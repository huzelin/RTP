#include "orc/framework/options.h"

namespace orc {

std::string Options::OrcDaemon          = "orc.app.daemon";
std::string Options::OrcInFile          = "orc.app.infile";
std::string Options::OrcOutFile         = "orc.app.outfile";
std::string Options::OrcErrFile         = "orc.app.errfile";
std::string Options::OrcLogMaxLength    = "orc.log.max.length";
std::string Options::OrcLogLevel        = "orc.log.level";

std::string Options::SvrName                = "svr.name";
std::string Options::SvrMaxRestartIntensity = "svr.restart.max.intensity";
std::string Options::SvrMaxRestartPeriod    = "svr.restart.max.period";
std::string Options::SvrIP                  = "svr.ip";
std::string Options::SvrPort                = "svr.port";
std::string Options::SvrIoThreadNum         = "svr.iothread.num";
std::string Options::SvrWorkerNum           = "svr.worker.num";
std::string Options::SvrWorkerQueueNum      = "svr.worker.queue.num";
std::string Options::SvrWorkerQueueSize     = "svr.worker.queue.size";
std::string Options::SvrWorkerPinCpu        = "svr.worker.pin.cpu";
std::string Options::SvrServiceDiscovery    = "svr.service.discovery";
std::string Options::SvrMonitorStatusFile   = "svr.monitor.status.file";
std::string Options::SvrWorkflowPoolMode    = "svr.workflow.pool.mode";
std::string Options::SvrWorkflowPoolSize    = "svr.workflow.pool.size";
std::string Options::SvrWorkflowUseTls      = "svr.workflow.use.tls";
std::string Options::SvrWorkflowFile        = "svr.workflow.file";
std::string Options::SvrHandlerFile         = "svr.handler.file";
std::string Options::SvrSessionFactory      = "svr.session.factory";
std::string Options::SvrSessionPoolSize     = "svr.session.pool.size";
std::string Options::SvrWorkflowRouter      = "svr.workflow.router";

std::string Options::ComEnableList          = "com.enable.list";
std::string Options::ComDisableList         = "com.disable.list";
std::string Options::ComRpcClientFile       = "com.rpc.client.file";
std::string Options::ComDataWorehouseFile   = "com.data.warehouse.file";
std::string Options::ComHttpClientFile      = "com.http.client.file";


std::string Options::PluginPath             = "plugin.path";

}  // namespace orc
