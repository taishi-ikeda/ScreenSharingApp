#include "screensharing/session_paths.h"

#include <sys/stat.h>

#include <QDir>
#include <QFileInfo>

namespace screensharing {
namespace SessionPaths {

namespace {
// Deliberately not QDir::temp(): on macOS that resolves to a per-user
// private directory ($TMPDIR, e.g. /var/folders/.../T/) which other OS
// accounts cannot see. The whole point of this directory is that it is
// reachable by every account on the machine, so it must be the literal
// shared /tmp (see SPEC.md section 5).
constexpr char kBaseDirPath[] = "/tmp/qt_screensharing";
}  // namespace

QString baseDir() { return QLatin1String(kBaseDirPath); }

QString sessionDir(const QString& code) { return QDir(baseDir()).filePath(code); }

QString metaFilePath(const QString& code) {
  return QDir(sessionDir(code)).filePath(QStringLiteral("meta.json"));
}

QString heartbeatFilePath(const QString& code) {
  return QDir(sessionDir(code)).filePath(QStringLiteral("heartbeat"));
}

QString frameFilePath(const QString& code, int bufferIndex) {
  return QDir(sessionDir(code)).filePath(QStringLiteral("frame_%1.bin").arg(bufferIndex));
}

bool ensureBaseDir() {
  const QString path = baseDir();
  if (QFileInfo(path).exists()) {
    // Already created, quite possibly by a different OS account's app
    // instance -- chmod() requires owning the file, so a non-owner can't
    // (and doesn't need to) redo the permissions here.
    return QFileInfo(path).isDir();
  }

  QDir dir;
  if (!dir.mkpath(path)) {
    return false;
  }
  // 01777: sticky + rwx for everyone, mirroring /tmp itself so any OS
  // account can create its own session subdirectory but only remove its own.
  return ::chmod(qPrintable(path), 01777) == 0;
}

}  // namespace SessionPaths
}  // namespace screensharing
