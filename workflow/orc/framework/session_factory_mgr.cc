#include "orc/framework/session_factory_mgr.h"

namespace orc {

SessionFactoryMgr* SessionFactoryMgr::Instance() {
  static SessionFactoryMgr mgr;
  return &mgr;
}

void SessionFactoryMgr::Register(const std::string& name,
                                 std::unique_ptr<SessionFactory> factory) {
  factories_[name] = std::move(factory);
}

SessionFactory* SessionFactoryMgr::GetSessionFactory(const std::string& name) {
  auto it = factories_.find(name);
  if (it == factories_.end()) return nullptr;
  return it->second.get();
}

}  // namespace orc
