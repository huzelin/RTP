#ifndef ORC_WORKFLOW_COMPILER_H__
#define ORC_WORKFLOW_COMPILER_H__

#include <string>
#include <map>
#include <memory>

#include "orc/util/macros.h"
#include "orc/workflow/graph.h"

namespace orc {
namespace wf {

class Compiler {
 public:
  Compiler();
  ~Compiler() = default;

  bool Compile(const std::string& file,
               std::map<std::string, std::unique_ptr<Graph>>* graphs);

 private:
  ORC_DISALLOW_COPY_AND_ASSIGN(Compiler);
};

}  // namespace wf
}  // namespace orc

#endif  // ORC_WORKFLOW_COMPILER_H__
