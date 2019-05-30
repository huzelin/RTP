#ifndef ORC_HANDLER_SYNC_HANDLER_H__
#define ORC_HANDLER_SYNC_HANDLER_H__

#include "orc/framework/handler_base.h"

namespace orc {

class SyncHandler : public HandlerBase {
 public:
  virtual bool Init(const YAML::Node& config) = 0;

  virtual bool Run(SessionBase* session_base) = 0;

 private:
  bool BaseInit(const YAML::Node& config) override;
  bool BaseRun(SessionBase* SessionBase, Context* context) override;
};

}  // namespace orc

#endif  // ORC_HANDLER_SYNC_HANDLER_H__
