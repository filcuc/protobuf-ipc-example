#include <QtCore>
#include "application.h"

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);
  protobuf_client_example::Application a;
  a.start("localhost", 56323);
  return app.exec();
}
