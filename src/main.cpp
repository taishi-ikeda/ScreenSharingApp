#include <QApplication>

#include "screensharing/main_window.h"

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  screensharing::MainWindow window;
  window.show();

  return app.exec();
}
