#include "orc/workflow/compiler.h"

#include <utility>
#include <vector>

#include "orc/workflow/parser/compiler.h"
#include "orc/workflow/graph.h"
#include "orc/workflow/node.h"
#include "orc/util/log.h"

namespace orc {
namespace wf {

namespace {

struct Edge {
  bool direct;
  Node* node;
};

class Label {
 public:
  Label() = default;
  ~Label() {
    if (srcs.size() != 0) {
      ORC_ERROR("Label not Attach");
    }
  }

  void AddEdge(Edge edge) {
    srcs.emplace_back(edge);
  }

  void Reset() { srcs.clear(); }

  void Attach(Node* node) {
    for (auto& edge : srcs) {
      if (edge.direct) {
        edge.node->set_true_edge(node);
      } else {
        edge.node->set_false_edge(node);
      }
    }
    srcs.clear();
  }

  void Append(Label* label) {
    for (auto& edge : label->srcs) {
      srcs.emplace_back(edge);
    }
    label->Reset();
  }

 private:
  std::vector<Edge> srcs;
};

struct Context {
  Context()
    : graphs(nullptr),
      graph(nullptr),
      entry(nullptr),
      prev_label(nullptr),
      next_label(nullptr) {}

  void Reset() {
    graph = nullptr;
    entry = nullptr;
    prev_label = nullptr;
    next_label = nullptr;
  }

  std::map<std::string, std::unique_ptr<Graph>>* graphs;
  Graph* graph;
  Node* entry;
  Label* prev_label;
  Label* next_label;
};

#define CHECK_KIND(syntax, ikind) \
do {\
  if (syntax->kind != ikind) { \
    ORC_ERROR("expect kind %d but %d.", ikind, syntax->kind); \
    return false; \
  } \
} while (0)

bool CompileExp(struct workflow_syntax*, Context*);
bool CompileBlock(struct workflow_syntax*, Context*);

bool CompileName(struct workflow_syntax* syntax, Context* ctx) {
  CHECK_KIND(syntax, WORKFLOW_SYNTAX_KIND_NAME);

  std::string name{syntax->u.name.str, static_cast<size_t>(syntax->u.name.size)};
  std::unique_ptr<Node> node{new Node(name)};

  ctx->prev_label->AddEdge({true, node.get()});
  ctx->next_label->AddEdge({false, node.get()});
  ctx->entry = node.get();

  ctx->graph->AddNode(std::move(node));
  return true;
}

bool CompileUnopExp(struct workflow_syntax* syntax, Context* ctx) {
  CHECK_KIND(syntax, WORKFLOW_SYNTAX_KIND_UNOP_EXP);
  switch (syntax->opkind) {
    case WORKFLOW_SYNTAX_OPKIND_NOT: {
      std::swap(ctx->prev_label, ctx->next_label);
      if (!CompileExp(syntax->u.unop.left, ctx)) {
        ORC_ERROR("Compile syntax unop fail.");
        return false;
      }
      return true;
    }

    default:
      ORC_ERROR("Unknown unop stmt opkind: %d", syntax->opkind);
      return false;
  }
  return false;
}

bool CompileExp(struct workflow_syntax* syntax, Context* ctx) {
  switch (syntax->kind) {
    case WORKFLOW_SYNTAX_KIND_UNOP_EXP:
      return CompileUnopExp(syntax, ctx);

    case WORKFLOW_SYNTAX_KIND_NAME:
      return CompileName(syntax, ctx);

    default:
      ORC_ERROR("Unknown exp syntax type.");
      return false;
  }
  return false;
}

bool CompileExpStmt(struct workflow_syntax* syntax, Context* ctx) {
  Context exp_ctx;
  exp_ctx.graph = ctx->graph;
  exp_ctx.prev_label = ctx->next_label;
  exp_ctx.next_label = ctx->next_label;
  if (!CompileExp(syntax, &exp_ctx)) {
    return false;
  }
  ctx->entry = exp_ctx.entry;
  ctx->prev_label->Attach(exp_ctx.entry);
  return true;
}

bool CompileReturnStmt(struct workflow_syntax* syntax, Context* ctx) {
  CHECK_KIND(syntax, WORKFLOW_SYNTAX_KIND_RETURN_STMT);
  ctx->prev_label->Attach(Node::EndNode());
  return true;
}

bool CompileIfStmt(struct workflow_syntax* syntax, Context* ctx) {
  CHECK_KIND(syntax, WORKFLOW_SYNTAX_KIND_IF_STMT);

  auto stx_condition = syntax->u.if_stmt.condition;
  auto stx_then = syntax->u.if_stmt.then_block;
  auto stx_else = syntax->u.if_stmt.else_block;

  Context condition_ctx;
  Label condition_end_label;
  Label then_start_label;
  condition_ctx.graph = ctx->graph;
  condition_ctx.prev_label = &then_start_label;
  condition_ctx.next_label = &condition_end_label;

  if (!CompileExp(stx_condition, &condition_ctx)) {
    ORC_ERROR("Compile syntax if condition fail.");
    return false;
  }

  Context then_ctx;
  then_ctx.graph = ctx->graph;
  then_ctx.prev_label = &then_start_label;
  then_ctx.next_label = ctx->next_label;
  if (!CompileBlock(stx_then, &then_ctx)) {
    ORC_ERROR("Compile syntax if then block fail.");
    return false;
  }

  then_start_label.Attach(then_ctx.entry);

  if (stx_else->kind != WORKFLOW_SYNTAX_KIND_EMPTY_STMT) {
    Context else_ctx;
    else_ctx.graph = ctx->graph;
    else_ctx.prev_label = &condition_end_label;
    else_ctx.next_label = ctx->next_label;

    if (!CompileBlock(stx_else, &else_ctx)) {
      ORC_ERROR("Compile syntax if else block fail.");
      return false;
    }
  } else {
    ctx->next_label->Append(&condition_end_label);
  }

  ctx->entry = condition_ctx.entry;
  ctx->prev_label->Attach(condition_ctx.entry);
  return true;
}

bool CompileWhileStmt(struct workflow_syntax* syntax, Context* ctx) {
  CHECK_KIND(syntax, WORKFLOW_SYNTAX_KIND_WHILE_STMT);

  auto stx_condition = syntax->u.while_stmt.condition;
  auto stx_block = syntax->u.while_stmt.block;

  Context condition_ctx;
  Label block_start_label;
  condition_ctx.graph = ctx->graph;
  condition_ctx.prev_label = &block_start_label;
  condition_ctx.next_label = ctx->next_label;

  if (!CompileExp(stx_condition, &condition_ctx)) {
    ORC_ERROR("Compile syntax while condition fail.");
    return false;
  }

  Context block_ctx;
  Label block_end_label;
  block_ctx.graph = ctx->graph;
  block_ctx.prev_label = &block_start_label;
  block_ctx.next_label = &block_end_label;

  if (!CompileBlock(stx_block, &block_ctx)) {
    ORC_ERROR("Compile sytanx while block fail.");
    return false;
  }

  block_start_label.Attach(block_ctx.entry);
  block_end_label.Attach(condition_ctx.entry);

  ctx->entry = condition_ctx.entry;
  ctx->prev_label->Attach(condition_ctx.entry);

  return true;
}

bool CompileDoWhileStmt(struct workflow_syntax* syntax, Context* ctx) {
  CHECK_KIND(syntax, WORKFLOW_SYNTAX_KIND_DOWHILE_STMT);

  auto stx_block = syntax->u.dowhile_stmt.block;
  auto stx_condition = syntax->u.dowhile_stmt.condition;

  Context block_ctx;
  Label block_end_label;
  block_ctx.graph = ctx->graph;
  block_ctx.prev_label = ctx->prev_label;
  block_ctx.next_label = &block_end_label;

  if (!CompileBlock(stx_block, &block_ctx)) {
    ORC_ERROR("Compile syntax dowhile block fail.");
    return false;
  }

  Context condition_ctx;
  Label true_label;
  Label false_label;

  condition_ctx.graph = ctx->graph;
  condition_ctx.prev_label = &true_label;
  condition_ctx.next_label = &false_label;

  if (!CompileExp(stx_condition, &condition_ctx)) {
    ORC_ERROR("Compile syntax dowhile condition fail.");
    return false;
  }

  true_label.Attach(block_ctx.entry);
  block_end_label.Attach(condition_ctx.entry);

  ctx->entry = block_ctx.entry;
  ctx->prev_label->Attach(block_ctx.entry);
  ctx->next_label->Append(&false_label);
  return true;
}

bool ComplieStmt(struct workflow_syntax* syntax, Context* ctx) {
  switch (syntax->kind) {
    case WORKFLOW_SYNTAX_KIND_IF_STMT:
      return CompileIfStmt(syntax, ctx);

    case WORKFLOW_SYNTAX_KIND_WHILE_STMT:
      return CompileWhileStmt(syntax, ctx);

    case WORKFLOW_SYNTAX_KIND_DOWHILE_STMT:
      return CompileDoWhileStmt(syntax, ctx);

    case WORKFLOW_SYNTAX_KIND_RETURN_STMT:
      return CompileReturnStmt(syntax, ctx);

    default: {
      return CompileExpStmt(syntax, ctx);
    }
  }

  ORC_ERROR("Unknow stmt type: %d", syntax->kind);
  return false;
}

bool CompileBlock(struct workflow_syntax* syntax, Context* ctx) {
  CHECK_KIND(syntax, WORKFLOW_SYNTAX_KIND_STMT_LIST);

  Context stmt_ctx;

  Label stmt_prev;
  Label stmt_next;

  stmt_ctx.graph = ctx->graph;
  stmt_ctx.entry = nullptr;
  stmt_ctx.prev_label = &stmt_prev;
  stmt_ctx.next_label = &stmt_next;

  for (auto i = 0; i < syntax->u.stmt_list.size; ++i) {
    if (!ComplieStmt(syntax->u.stmt_list.child[i], &stmt_ctx)) {
      return false;
    }

    if (i == 0) {
      ctx->entry = stmt_ctx.entry;
    }

    std::swap(stmt_ctx.prev_label, stmt_ctx.next_label);

    if (syntax->u.stmt_list.child[i]->kind == WORKFLOW_SYNTAX_KIND_RETURN_STMT) {
      break;
    }
  }

  ctx->prev_label->Attach(ctx->entry);
  ctx->next_label->Append(stmt_ctx.prev_label);
  return true;
}

bool CompileWorkflow(struct workflow_syntax* syntax, Context* ctx) {
  CHECK_KIND(syntax, WORKFLOW_SYNTAX_KIND_WORKFLOW);

  auto stx_name = syntax->u.workflow_stmt.name;
  CHECK_KIND(stx_name, WORKFLOW_SYNTAX_KIND_NAME);

  std::string name{stx_name->u.name.str, static_cast<size_t>(stx_name->u.name.size)};
  if (ctx->graphs->find(name) != ctx->graphs->end()) {
    ORC_ERROR("Workflow has duplicate name: %s.", name.c_str());
    return false;
  }

  std::unique_ptr<Graph> graph{new Graph(name)};

  Label prev_label;
  Label next_label;

  ctx->Reset();
  ctx->graph = graph.get();
  ctx->prev_label = &prev_label;
  ctx->next_label = &next_label;

  if (!CompileBlock(syntax->u.workflow_stmt.block, ctx)) {
    ORC_ERROR("Workflow %s compile fail.", name.c_str());
    return false;
  }

  ctx->next_label->Attach(Node::EndNode());
  graph->set_entry(ctx->entry);

  if (!graph->CheckValid()) {
    ORC_ERROR("Workflow %s is not valid graph.", name.c_str());
    return false;
  }

  ctx->graphs->emplace(name, std::move(graph));
  ORC_INFO("Workflow %s compile success", name.c_str());
  return true;
}

bool CompileInner(struct workflow_syntax* syntax,
                  std::map<std::string, std::unique_ptr<Graph>>* graphs) {
  CHECK_KIND(syntax, WORKFLOW_SYNTAX_KIND_STMT_LIST);

  Context ctx;
  ctx.graphs = graphs;
  for (auto i = 0; i < syntax->u.stmt_list.size; ++i) {
    if (!CompileWorkflow(syntax->u.stmt_list.child[i], &ctx)) {
      return false;
    }
  }

  return true;
}

}  // namespace

static void error_report(const char* format, va_list va) {
  Log(LogLevel::Error, "parser", 0, "compiler", format, va);
}

Compiler::Compiler() {
  workflow_set_compiler_error_reporter(&error_report);
}

bool Compiler::Compile(const std::string& file,
                       std::map<std::string, std::unique_ptr<Graph>>* graphs) {
  std::unique_ptr<struct workflow_syntax, void(*)(struct workflow_syntax*)> node{
     workflow_syntax_new_stmt_list(), workflow_syntax_delete_node
  };

  if (workflow_compile(file.c_str(), node.get()) != 0) {
    ORC_ERROR("Compile workflow file: %s fail.", file.c_str());
    return false;
  }

  if (!CompileInner(node.get(), graphs)) {
    ORC_ERROR("Syntax analysis fail.");
    return false;
  }

  ORC_INFO("Compile successfully.");
  return true;
}

}  // namespace wf
}  // namespace orc
