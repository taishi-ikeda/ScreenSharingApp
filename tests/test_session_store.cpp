#include <QtTest/QtTest>

#include "screensharing/invite_code.h"
#include "screensharing/session_reader.h"
#include "screensharing/session_writer.h"

namespace screensharing {
namespace {

class SessionStoreTest : public QObject {
  Q_OBJECT

 private slots:
  void writtenFrameIsReadableAfterStart();
  void secondFrameFlipsBufferAndStaysReadable();
  void readerIsNotAliveBeforeStart();
  void stopRemovesSessionDirectory();
  void generatedInviteCodeIsFourDigitsAndUnused();
};

void SessionStoreTest::writtenFrameIsReadableAfterStart() {
  const QString code = QStringLiteral("0001");
  SessionWriter writer(code);
  QVERIFY(writer.start());

  const QByteArray payload = QByteArrayLiteral("hello-frame");
  QVERIFY(writer.writeFrame(payload, 640, 480));

  SessionReader reader(code);
  QVERIFY(reader.isAlive());

  QByteArray readBack;
  int width = 0;
  int height = 0;
  QVERIFY(reader.readFrame(&readBack, &width, &height));
  QCOMPARE(readBack, payload);
  QCOMPARE(width, 640);
  QCOMPARE(height, 480);

  writer.stop();
}

void SessionStoreTest::secondFrameFlipsBufferAndStaysReadable() {
  const QString code = QStringLiteral("0002");
  SessionWriter writer(code);
  QVERIFY(writer.start());

  QVERIFY(writer.writeFrame(QByteArrayLiteral("frame-a"), 100, 100));
  QVERIFY(writer.writeFrame(QByteArrayLiteral("frame-b"), 200, 200));

  SessionReader reader(code);
  QByteArray readBack;
  int width = 0;
  int height = 0;
  QVERIFY(reader.readFrame(&readBack, &width, &height));
  QCOMPARE(readBack, QByteArrayLiteral("frame-b"));
  QCOMPARE(width, 200);
  QCOMPARE(height, 200);

  writer.stop();
}

void SessionStoreTest::readerIsNotAliveBeforeStart() {
  SessionReader reader(QStringLiteral("0003"));
  QVERIFY(!reader.isAlive());
}

void SessionStoreTest::stopRemovesSessionDirectory() {
  const QString code = QStringLiteral("0004");
  SessionWriter writer(code);
  QVERIFY(writer.start());
  writer.stop();

  SessionReader reader(code);
  QVERIFY(!reader.isAlive());
}

void SessionStoreTest::generatedInviteCodeIsFourDigitsAndUnused() {
  const QString code = generateAvailableInviteCode();
  QCOMPARE(code.size(), 4);
  QVERIFY(!SessionReader(code).isAlive());
}

}  // namespace
}  // namespace screensharing

QTEST_MAIN(screensharing::SessionStoreTest)
#include "test_session_store.moc"
