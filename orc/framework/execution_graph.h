#ifndef ORC_FRAMEWORK_EXECUTION_GRAPH_H_
#define ORC_FRAMEWORK_EXECUTION_GRAPH_H_

#include <map>
#include <string>
#include <memory>

#include "orc/util/macros.h"
#include "orc/framework/execution_node.h"

namespace orc {

class ExecutionGraph {
 public:
  explicit ExecutionGraph(const std::string& name) : name_(name), entry_(nullptr) {}
  ~ExecutionGraph() = default;

  std::unique_ptr<ExecutionGraph> Clone() const;

  ExecutionNode* entry() const { return entry_; }
  void set_entry(ExecutionNode* entry) { entry_ = entry; }

  const std::string& name() const { return name_; }

  void set_version(uint32_t version) { version_ = version; }
  uint32_t version() const { return version_; }

  void AddNode(const std::string& name, std::unique_ptr<ExecutionNode> node);
  ExecutionNode* GetNode(const std::string& name) const;

 private:
  std::string name_;
  uint32_t version_;

  ExecutionNode* entry_;
  std::map<std::string, std::unique_ptr<ExecutionNode>> nodes_;

  ORC_DISALLOW_COPY_AND_ASSIGN(ExecutionGraph);
};

}  // namespace orc


#endif  // ORC_FRAMEWORK_EXECUTION_GRAPH_H_
