#include <iostream>

#include "orc/workflow/parser/compiler.h"
#include "orc/workflow/parser/syntax.h"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << argv[0] << " <file.wf>" << std::endl;
    return -1;
  }

  const char* file = argv[1];

  auto node = workflow_syntax_new_stmt_list();
  if (workflow_compile(file, node) != 0) {
    std::cout << "compile fail" << std::endl;
    return -1;
  }

  workflow_syntax_print_node(node);
  return 0;
}
