#ifndef EXAMPLE_ORC_SERVER_HANDLER_HELLO_HANDLER_H_
#define EXAMPLE_ORC_SERVER_HANDLER_HELLO_HANDLER_H_

#include "orc/handler/sync_handler.h"
#include "../hello_session.h"

namespace orc {
namespace example {
namespace hello {

class HelloHandler : public SyncHandler {
 public:
  bool Init(const YAML::Node& config) override;
  bool Run(orc::SessionBase* session_base) override;
};

}  // namespace hello
}  // namespace example
}  // namespace orc

#endif  // EXAMPLE_ORC_SERVER_HANDLER_HELLO_HANDLER_H_
