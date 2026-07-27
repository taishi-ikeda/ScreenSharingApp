#include <QtTest/QtTest>

#include <QDir>

#include "screensharing/presence.h"
#include "screensharing/session_paths.h"

namespace screensharing {
namespace {

class PresenceTest : public QObject {
  Q_OBJECT

 private slots:
  void init();

  void activeUsersIncludesSelfWhileAlive();
  void destructorRemovesBaseDirWhenLastUserLeaves();
};

void PresenceTest::init() { QDir(SessionPaths::baseDir()).removeRecursively(); }

void PresenceTest::activeUsersIncludesSelfWhileAlive() {
  Presence presence;
  QVERIFY(Presence::activeUsers().contains(qEnvironmentVariable("USER")));
}

void PresenceTest::destructorRemovesBaseDirWhenLastUserLeaves() {
  {
    Presence presence;
    QVERIFY(QDir(SessionPaths::baseDir()).exists());
  }
  // Presence went out of scope (destructor ran) and was the only user, so
  // the whole shared base directory should be gone, not just this user's
  // own presence file.
  QVERIFY(!QDir(SessionPaths::baseDir()).exists());
}

}  // namespace
}  // namespace screensharing

QTEST_MAIN(screensharing::PresenceTest)
#include "test_presence.moc"
