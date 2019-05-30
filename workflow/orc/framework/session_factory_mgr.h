#ifndef ORC_FRAMEWORK_SESSION_FACTORY_MGR_H_
#define ORC_FRAMEWORK_SESSION_FACTORY_MGR_H_

#include <string>
#include <map>
#include <functional>
#include <memory>

#include "orc/util/macros.h"
#include "orc/framework/session_factory.h"

namespace orc {

class SessionFactoryMgr {
 public:
  ~SessionFactoryMgr() = default;

  static SessionFactoryMgr* Instance();

  void Register(const std::string& name, std::unique_ptr<SessionFactory> factory);

  SessionFactory* GetSessionFactory(const std::string& name);

 private:
  SessionFactoryMgr() = default;

 private:
  std::map<std::string, std::unique_ptr<SessionFactory>> factories_;

  ORC_DISALLOW_COPY_AND_ASSIGN(SessionFactoryMgr);
};

}  // namespace orc

#define ORC_REGISTER_SESSION_FACTORY(Factory) \
__attribute__((constructor)) void orc_register_session_factory_##Factory() { \
  orc::SessionFactoryMgr::Instance()->Register( \
      #Factory, std::unique_ptr<orc::SessionFactory>(new Factory())); \
}

#endif  // ORC_FRAMEWORK_SESSION_FACTORY_MGR_H_
