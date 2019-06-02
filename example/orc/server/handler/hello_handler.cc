#include "hello_handler.h"

#include "orc/framework/handler_mgr.h"

namespace orc {
namespace example {
namespace hello {

ORC_REGISTER_HANDLER(HelloHandler);

bool HelloHandler::Init(const YAML::Node& config) {
  return true;
}

bool HelloHandler::Run(orc::SessionBase* session_base) {
  auto session = static_cast<HelloSession*>(session_base);
  auto request = session->request();

  std::string out = "Hello ";
  if (!request->has_name() || request->name().empty()) {
    out += "World";
  } else {
    out += request->name();
  }

  auto response = session->response();
  response->set_content(out);
  return true;
}

}  // namespace hello
}  // namespace example
}  // namespace orc
