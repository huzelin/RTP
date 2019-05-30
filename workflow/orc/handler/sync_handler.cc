#include "orc/handler/sync_handler.h"

namespace orc {

bool SyncHandler::BaseInit(const YAML::Node& config) {
  return Init(config);
}

bool SyncHandler::BaseRun(SessionBase* session_base, Context* context) {
  context->SetAsync(false);
  return Run(session_base);
}

}
