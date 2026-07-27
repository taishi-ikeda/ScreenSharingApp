#include <QtTest/QtTest>

#include "screensharing/main_window.h"

namespace screensharing {
namespace {

// Placeholder: verifies the GUI shell builds and instantiates.
// Real behavioral tests are added once session logic is implemented.
class SmokeTest : public QObject {
  Q_OBJECT

 private slots:
  void mainWindowConstructs();
};

void SmokeTest::mainWindowConstructs() {
  MainWindow window;
  QVERIFY(window.centralWidget() != nullptr);
}

}  // namespace
}  // namespace screensharing

QTEST_MAIN(screensharing::SmokeTest)
#include "test_smoke.moc"
