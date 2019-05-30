#ifndef ORC_FRAMEWORK_SERVER_MGR_H_
#define ORC_FRAMEWORK_SERVER_MGR_H_

#include <string>
#include <memory>

#include "orc/util/macros.h"
#include "orc/framework/server_base.h"

namespace orc {

class ServerMgr {
 public:
  ~ServerMgr() = default;
  static ServerMgr* Instance();

  void Register(const std::string& name, std::unique_ptr<ServerBase> server);

  bool StartServer(const std::string& name, const YAML::Node& config);
  void StopServer();

 private:
  ServerMgr() = default;

 private:
  std::map<std::string, std::unique_ptr<ServerBase>> servers_;
  std::string started_server_;

  ORC_DISALLOW_COPY_AND_ASSIGN(ServerMgr);
};

}  // namespace orc

#define ORC_REGISTER_SERVER(Server) \
__attribute__((constructor)) void orc_register_server_##Server() { \
  orc::ServerMgr::Instance()->Register( \
      #Server, std::unique_ptr<orc::ServerBase>(new Server())); \
}

#endif  // ORC_FRAMEWORK_SERVER_MGR_H_
