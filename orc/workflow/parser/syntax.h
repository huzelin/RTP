#ifndef ORC_WORKFLOW_PARSER_WORKFLOW_SYNTAX_H__
#define ORC_WORKFLOW_PARSER_WORKFLOW_SYNTAX_H__


#ifdef __cplusplus
extern "C" {
#endif

enum {
  WORKFLOW_SYNTAX_OPKIND_NOT,
};

enum {
  WORKFLOW_SYNTAX_KIND_EMPTY_STMT = 0,
  WORKFLOW_SYNTAX_KIND_NAME,
  WORKFLOW_SYNTAX_KIND_WORKFLOW,
  WORKFLOW_SYNTAX_KIND_STMT_LIST,
  WORKFLOW_SYNTAX_KIND_IF_STMT,
  WORKFLOW_SYNTAX_KIND_WHILE_STMT,
  WORKFLOW_SYNTAX_KIND_DOWHILE_STMT,
  WORKFLOW_SYNTAX_KIND_RETURN_STMT,
  WORKFLOW_SYNTAX_KIND_UNOP_EXP,
};

struct workflow_syntax {
  int kind;
  int opkind;

  union {
    struct {
      char* str;
      int size;
    } name;

    struct {
      struct workflow_syntax* name;
      struct workflow_syntax* block;
    } workflow_stmt;

    struct {
      struct workflow_syntax** child;
      int capacity;
      int size;
    } stmt_list;

    struct {
      struct workflow_syntax* condition;
      struct workflow_syntax* then_block;
      struct workflow_syntax* else_block;
    } if_stmt;

    struct {
      struct workflow_syntax* condition;
      struct workflow_syntax* block;
    } while_stmt;

    struct {
      struct workflow_syntax* block;
      struct workflow_syntax* condition;
    } dowhile_stmt;

    struct {
      struct workflow_syntax* left;
    } unop;
  } u;
};

struct workflow_syntax* workflow_syntax_new_node(int kind);
void workflow_syntax_delete_node(struct workflow_syntax* node);

void workflow_syntax_add_stmt(struct workflow_syntax* self,
                              struct workflow_syntax* stmt);

// workflow_syntax unit constructor
struct workflow_syntax* workflow_syntax_new_workflow(
    struct workflow_syntax* name,
    struct workflow_syntax* block);

struct workflow_syntax* workflow_syntax_new_stmt_list();

struct workflow_syntax* workflow_syntax_new_empty_stmt();

struct workflow_syntax* workflow_syntax_new_if_stmt(
    struct workflow_syntax* condition,
    struct workflow_syntax* then_block,
    struct workflow_syntax* else_block);

struct workflow_syntax* workflow_syntax_new_while_stmt(
    struct workflow_syntax* condition,
    struct workflow_syntax* block);

struct workflow_syntax* workflow_syntax_new_dowhile_stmt(
    struct workflow_syntax* block,
    struct workflow_syntax* condition);

struct workflow_syntax* workflow_syntax_new_return_stmt();

struct workflow_syntax* workflow_syntax_new_unop_exp(
    int opkind,
    struct workflow_syntax* exp);

struct workflow_syntax* workflow_syntax_new_name_stmt(char* name);


// for debug
void workflow_syntax_print_node(struct workflow_syntax* node);


#ifdef __cplusplus
}
#endif

#endif  // ORC_WORKFLOW_PARSER_WORKFLOW_SYNTAX_H__
