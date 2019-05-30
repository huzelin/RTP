#include "orc/framework/application.h"

int main(int argc, char* argv[]) {
  orc::Application app;
  return app.Run(argc, argv) ? 0 : -1;
}
