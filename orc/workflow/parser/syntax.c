#include "orc/workflow/parser/syntax.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "orc/workflow/parser/compiler.h"

struct workflow_syntax* workflow_syntax_new_node(int kind) {
  struct workflow_syntax* node = calloc(1, sizeof(*node));
  if (node == NULL) {
    workflow_compiler_error_report("new syntax node %d fail.", kind);
    return NULL;
  }

  node->kind = kind;
  return node;
}

void workflow_syntax_delete_node(struct workflow_syntax* node) {
  int i = 0;
  switch(node->kind) {
    case WORKFLOW_SYNTAX_KIND_EMPTY_STMT:
      break;

    case WORKFLOW_SYNTAX_KIND_NAME:
      free(node->u.name.str);
      break;

    case WORKFLOW_SYNTAX_KIND_WORKFLOW:
      workflow_syntax_delete_node(node->u.workflow_stmt.name);
      workflow_syntax_delete_node(node->u.workflow_stmt.block);
      break;

    case WORKFLOW_SYNTAX_KIND_STMT_LIST:
      for (i = 0; i < node->u.stmt_list.size; ++i) {
        workflow_syntax_delete_node(node->u.stmt_list.child[i]);
      }
      free(node->u.stmt_list.child);
      break;

    case WORKFLOW_SYNTAX_KIND_IF_STMT:
      workflow_syntax_delete_node(node->u.if_stmt.condition);
      workflow_syntax_delete_node(node->u.if_stmt.then_block);
      workflow_syntax_delete_node(node->u.if_stmt.else_block);
      break;

    case WORKFLOW_SYNTAX_KIND_WHILE_STMT:
      workflow_syntax_delete_node(node->u.while_stmt.condition);
      workflow_syntax_delete_node(node->u.while_stmt.block);
      break;

    case WORKFLOW_SYNTAX_KIND_DOWHILE_STMT:
      workflow_syntax_delete_node(node->u.dowhile_stmt.block);
      workflow_syntax_delete_node(node->u.dowhile_stmt.condition);
      break;

    case WORKFLOW_SYNTAX_KIND_UNOP_EXP:
      workflow_syntax_delete_node(node->u.unop.left);
      break;

    default:
      break;
  }
  free(node);
}

void workflow_syntax_add_stmt(struct workflow_syntax* self,
    struct workflow_syntax* stmt) {

  if (self->u.stmt_list.child == NULL) {
    self->u.stmt_list.child = calloc(10,  sizeof (struct workflow_syntax*));
    self->u.stmt_list.capacity = 10;
  }

  if (self->u.stmt_list.size >= self->u.stmt_list.capacity) {
    self->u.stmt_list.capacity = 2 * self->u.stmt_list.capacity;
    int new_raw_size  =
      self->u.stmt_list.capacity * sizeof(struct workflow_syntax*);
    self->u.stmt_list.child = realloc(self->u.stmt_list.child, new_raw_size);
  }

  self->u.stmt_list.child[self->u.stmt_list.size++] = stmt;
}

struct workflow_syntax* workflow_syntax_new_workflow(
    struct workflow_syntax* name,
    struct workflow_syntax* block) {
  struct workflow_syntax* workflow =
    workflow_syntax_new_node(WORKFLOW_SYNTAX_KIND_WORKFLOW);
  if (workflow == NULL) {
    workflow_compiler_error_report("new workflow node fail.");
    return NULL;
  }
  workflow->u.workflow_stmt.name = name;
  workflow->u.workflow_stmt.block = block;
  return workflow;
}

struct workflow_syntax* workflow_syntax_new_stmt_list() {
  return workflow_syntax_new_node(WORKFLOW_SYNTAX_KIND_STMT_LIST);
}

struct workflow_syntax* workflow_syntax_new_empty_stmt() {
  return workflow_syntax_new_node(WORKFLOW_SYNTAX_KIND_EMPTY_STMT);
}

struct workflow_syntax* workflow_syntax_new_if_stmt(
    struct workflow_syntax* condition,
    struct workflow_syntax* then_block,
    struct workflow_syntax* else_block) {
  struct workflow_syntax* if_stmt =
    workflow_syntax_new_node(WORKFLOW_SYNTAX_KIND_IF_STMT);
  if (if_stmt == NULL) {
    return NULL;
  }

  if_stmt->u.if_stmt.condition = condition;
  if_stmt->u.if_stmt.then_block = then_block;
  if_stmt->u.if_stmt.else_block = else_block;
  return if_stmt;
}

struct workflow_syntax* workflow_syntax_new_while_stmt(
    struct workflow_syntax* condition,
    struct workflow_syntax* block) {
  struct workflow_syntax* while_stmt =
    workflow_syntax_new_node(WORKFLOW_SYNTAX_KIND_WHILE_STMT);
  if (while_stmt == NULL) {
    return NULL;
  }

  while_stmt->u.while_stmt.condition = condition;
  while_stmt->u.while_stmt.block = block;
  return while_stmt;
}

struct workflow_syntax* workflow_syntax_new_dowhile_stmt(
    struct workflow_syntax* block,
    struct workflow_syntax* condition) {
  struct workflow_syntax* dowhile_stmt =
    workflow_syntax_new_node(WORKFLOW_SYNTAX_KIND_DOWHILE_STMT);
  if (dowhile_stmt == NULL) {
    return NULL;
  }

  dowhile_stmt->u.dowhile_stmt.block = block;
  dowhile_stmt->u.dowhile_stmt.condition = condition;
  return dowhile_stmt;
}

struct workflow_syntax* workflow_syntax_new_return_stmt() {
  return workflow_syntax_new_node(WORKFLOW_SYNTAX_KIND_RETURN_STMT);
}

struct workflow_syntax* workflow_syntax_new_unop_exp(
    int opkind,
    struct workflow_syntax* exp) {
  struct workflow_syntax* unop_stmt =
    workflow_syntax_new_node(WORKFLOW_SYNTAX_KIND_UNOP_EXP);
  if (unop_stmt == NULL) {
    return NULL;
  }

  unop_stmt->opkind = WORKFLOW_SYNTAX_OPKIND_NOT;
  unop_stmt->u.unop.left = exp;
  return unop_stmt;
}

struct workflow_syntax* workflow_syntax_new_name_stmt(char* name) {
  struct workflow_syntax* name_stmt =
    workflow_syntax_new_node(WORKFLOW_SYNTAX_KIND_NAME);
  if (name_stmt == NULL) {
    return NULL;
  }

  name_stmt->u.name.str = strdup(name);
  name_stmt->u.name.size = strlen(name);
  return name_stmt;
}

static void workflow_syntax_print_unop_node(struct workflow_syntax* node) {
  switch (node->opkind) {
    case WORKFLOW_SYNTAX_OPKIND_NOT:
      printf("!");
      workflow_syntax_print_node(node->u.unop.left);
      break;

    default:
      break;
  }
}

void workflow_syntax_print_node(struct workflow_syntax* node) {
  switch (node->kind) {
    case WORKFLOW_SYNTAX_KIND_WORKFLOW:
      printf("workflow ");
      workflow_syntax_print_node(node->u.workflow_stmt.name);
      workflow_syntax_print_node(node->u.workflow_stmt.block);
      break;

    case WORKFLOW_SYNTAX_KIND_NAME:
      printf("%s\n", node->u.name.str);
      break;

    case WORKFLOW_SYNTAX_KIND_STMT_LIST:
      printf("{\n");
      int i;
      for (i = 0; i < node->u.stmt_list.size; ++i) {
        workflow_syntax_print_node(node->u.stmt_list.child[i]);
      }
      printf("}\n");
      break;

    case WORKFLOW_SYNTAX_KIND_IF_STMT:
      printf("if ");
      workflow_syntax_print_node(node->u.if_stmt.condition);
      workflow_syntax_print_node(node->u.if_stmt.then_block);
      if (node->u.if_stmt.else_block != NULL &&
          node->u.if_stmt.else_block->kind != WORKFLOW_SYNTAX_KIND_EMPTY_STMT) {
        printf("else\n");
        workflow_syntax_print_node(node->u.if_stmt.else_block);
      }
      break;

    case WORKFLOW_SYNTAX_KIND_WHILE_STMT:
      printf("while ");
      workflow_syntax_print_node(node->u.while_stmt.condition);
      workflow_syntax_print_node(node->u.while_stmt.block);
      break;

    case WORKFLOW_SYNTAX_KIND_DOWHILE_STMT:
      printf("do ");
      workflow_syntax_print_node(node->u.dowhile_stmt.block);
      printf("while ");
      workflow_syntax_print_node(node->u.dowhile_stmt.condition);
      break;

    case WORKFLOW_SYNTAX_KIND_RETURN_STMT:
      printf("return\n");
      break;

    case WORKFLOW_SYNTAX_KIND_UNOP_EXP:
      workflow_syntax_print_unop_node(node);
      break;

    case WORKFLOW_SYNTAX_KIND_EMPTY_STMT:
    default:
      break;
  }
}
