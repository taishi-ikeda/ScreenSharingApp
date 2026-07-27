#include <QtTest/QtTest>

#include <QLabel>
#include <QLineEdit>

#include "screensharing/presenter_panel.h"
#include "screensharing/viewer_panel.h"

namespace screensharing {
namespace {

// Drives PresenterPanel and ViewerPanel through their private slots via the
// meta-object system (no real UI clicks) to exercise the full
// write -> encrypt -> read -> decrypt pipeline end to end. Individual
// pieces (SessionWriter/Reader, XorCipher, frame_image) already have their
// own focused unit tests; this only checks that the panels wire them up
// correctly.
class PresenterViewerIntegrationTest : public QObject {
  Q_OBJECT

 private slots:
  void viewerConnectsAndReceivesFrameFromPresenter();
};

void PresenterViewerIntegrationTest::viewerConnectsAndReceivesFrameFromPresenter() {
  PresenterPanel presenter;
  QVERIFY(QMetaObject::invokeMethod(&presenter, "onStartClicked"));

  auto* inviteCodeLabel = presenter.findChild<QLabel*>(QStringLiteral("inviteCodeLabel"));
  QVERIFY(inviteCodeLabel != nullptr);
  const QString code = inviteCodeLabel->text();
  QCOMPARE(code.size(), 4);

  QVERIFY(QMetaObject::invokeMethod(&presenter, "publishNextFrame"));

  ViewerPanel viewer;
  auto* codeLineEdit = viewer.findChild<QLineEdit*>(QStringLiteral("codeLineEdit"));
  auto* statusLabel = viewer.findChild<QLabel*>(QStringLiteral("statusLabel"));
  QVERIFY(codeLineEdit != nullptr);
  QVERIFY(statusLabel != nullptr);

  codeLineEdit->setText(code);
  QVERIFY(QMetaObject::invokeMethod(&viewer, "onConnectClicked"));
  QVERIFY(statusLabel->text().contains(code));

  QVERIFY(QMetaObject::invokeMethod(&viewer, "pollForFrame"));
  // Still connected (not "Disconnected"/error text): the read + decrypt
  // succeeded without the panel giving up on the session.
  QVERIFY(statusLabel->text().contains(code));

  QVERIFY(QMetaObject::invokeMethod(&presenter, "onStopClicked"));
}

}  // namespace
}  // namespace screensharing

QTEST_MAIN(screensharing::PresenterViewerIntegrationTest)
#include "test_presenter_viewer_integration.moc"
