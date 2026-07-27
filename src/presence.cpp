#include "screensharing/presence.h"

#include <sys/stat.h>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTimer>

#include "screensharing/session_paths.h"

namespace screensharing {

namespace {

constexpr int kHeartbeatIntervalMs = 2000;
constexpr qint64 kStaleMs = 5000;

QString presenceDir() {
  return QDir(SessionPaths::baseDir()).filePath(QStringLiteral("presence"));
}

QString presenceFilePath(const QString& username) {
  return QDir(presenceDir()).filePath(username);
}

QString currentUsername() {
  const QString name = qEnvironmentVariable("USER");
  return name.isEmpty() ? QStringLiteral("unknown") : name;
}

}  // namespace

Presence::Presence(QObject* parent) : QObject(parent), heartbeatTimer_(new QTimer(this)) {
  SessionPaths::ensureBaseDir();

  QDir dir;
  dir.mkpath(presenceDir());
  ::chmod(qPrintable(presenceDir()), 01777);

  touchHeartbeat();

  heartbeatTimer_->setInterval(kHeartbeatIntervalMs);
  connect(heartbeatTimer_, &QTimer::timeout, this, &Presence::touchHeartbeat);
  heartbeatTimer_->start();
}

Presence::~Presence() {
  QFile::remove(presenceFilePath(currentUsername()));

  // If no other account's app instance is still running, remove the
  // shared base directory entirely rather than leaving empty leftover
  // directories (and any stale session dirs from crashed instances) in
  // /tmp indefinitely.
  if (activeUsers().isEmpty()) {
    QDir(SessionPaths::baseDir()).removeRecursively();
  }
}

void Presence::touchHeartbeat() {
  const QString path = presenceFilePath(currentUsername());
  QFile file(path);
  if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    file.write(QByteArray::number(QDateTime::currentMSecsSinceEpoch()));
    file.close();
    ::chmod(qPrintable(path), 0644);
  }
}

QStringList Presence::activeUsers() {
  QStringList result;
  const QDir dir(presenceDir());
  for (const QString& username : dir.entryList(QDir::Files)) {
    QFile file(dir.filePath(username));
    if (!file.open(QIODevice::ReadOnly)) {
      continue;
    }
    bool ok = false;
    const qint64 lastBeatMs = file.readAll().toLongLong(&ok);
    if (ok && (QDateTime::currentMSecsSinceEpoch() - lastBeatMs) < kStaleMs) {
      result.append(username);
    }
  }
  return result;
}

}  // namespace screensharing
