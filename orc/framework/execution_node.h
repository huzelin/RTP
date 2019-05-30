#ifndef ORC_FRAMEWORK_EXECUTION_NODE_H_
#define ORC_FRAMEWORK_EXECUTION_NODE_H_

#include "orc/util/macros.h"
#include "orc/framework/handler_base.h"
#include "orc/framework/session_base.h"
#include "orc/framework/context.h"

namespace orc {

class ExecutionNode {
 public:
  explicit ExecutionNode(const std::string& name)
      : name_(name), true_next_(nullptr), false_next_(nullptr), handler_(nullptr) {}
  ~ExecutionNode() = default;

  static ExecutionNode* EndNode();

  // Clone a ExecutionNode, leave 'true_next' and 'false_next' not set.
  std::unique_ptr<ExecutionNode> Clone() const;

  void Run(SessionBase* session, Context* ctx);

  void set_handler(std::unique_ptr<HandlerBase> handler) { handler_ = std::move(handler); }
  HandlerBase* handler() const { return handler_.get(); }

  void set_true_next(ExecutionNode* next) { true_next_ = next; }
  ExecutionNode* true_next() const { return true_next_; }

  void set_false_next(ExecutionNode* next) { false_next_ = next; }
  ExecutionNode* false_next() const { return false_next_; }

  const std::string& name() const { return name_; }

 private:
  std::string name_;
  ExecutionNode* true_next_;
  ExecutionNode* false_next_;

  std::unique_ptr<HandlerBase> handler_;

  ORC_DISALLOW_COPY_AND_ASSIGN(ExecutionNode);
};

}  // namespace orc


#endif  // ORC_FRAMEWORK_EXECUTION_NODE_H_
