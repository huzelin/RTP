#ifndef ORC_COM_COMPONENT_MGR_H_
#define ORC_COM_COMPONENT_MGR_H_

#include <string>
#include <memory>
#include <map>

#include "orc/util/macros.h"
#include "yaml-cpp/yaml.h"

namespace orc {

class Component {
 public:
  Component() = default;
  virtual ~Component() = default;

  virtual bool Setup(const YAML::Node& config) = 0;
  virtual void Release() = 0;
};

class ComponentMgr {
 public:
  ~ComponentMgr() = default;
  static ComponentMgr* Instance();

  bool Setup(const YAML::Node& config);
  void Release();

  void Register(const std::string& name, std::unique_ptr<Component> com);

 private:
  ComponentMgr() = default;

 private:
  std::map<std::string, std::unique_ptr<Component>> components_;

  ORC_DISALLOW_COPY_AND_ASSIGN(ComponentMgr);
};

}  // namespace orc

#define ORC_REGISTER_COM(Com) \
__attribute__((constructor)) void orc_register_com_##Com() { \
  orc::ComponentMgr::Instance()->Register( \
      #Com, std::unique_ptr<orc::Component>(new Com())); \
}

#endif  // ORC_COM_COMPONENT_MGR_H_
