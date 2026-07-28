#include "screensharing/session_writer.h"

#include <sys/stat.h>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

#include "screensharing/session_paths.h"

namespace screensharing {

namespace {

bool writeAtomically(const QString& path, const QByteArray& data) {
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    return false;
  }
  file.write(data);
  if (!file.commit()) {
    return false;
  }
  // World-readable: cross-account viewers have no shared group to rely on.
  return ::chmod(qPrintable(path), 0644) == 0;
}

}  // namespace

SessionWriter::SessionWriter(const QString& code)
    : code_(code), nextBufferIndex_(0), started_(false) {}

SessionWriter::~SessionWriter() { stop(); }

bool SessionWriter::start() {
  if (!SessionPaths::ensureBaseDir()) {
    return false;
  }

  const QString path = SessionPaths::sessionDir(code_);
  const QFileInfo info(path);
  if (info.exists()) {
    // A stale directory from a crashed session (possibly a different OS
    // account's) happened to get the same random invite code. chmod()
    // requires owning it, so a non-owner can't (and doesn't need to) redo
    // the permissions; just reuse it as-is.
    if (!info.isDir()) {
      return false;
    }
  } else {
    QDir dir;
    if (!dir.mkpath(path)) {
      return false;
    }
    if (::chmod(qPrintable(path), 0755) != 0) {
      return false;
    }
  }

  started_ = true;
  touchHeartbeat();
  return true;
}

bool SessionWriter::writeFrame(const QByteArray& encryptedFrame, int width, int height) {
  if (!started_) {
    return false;
  }

  const int bufferIndex = nextBufferIndex_;
  nextBufferIndex_ = 1 - nextBufferIndex_;

  if (!writeAtomically(SessionPaths::frameFilePath(code_, bufferIndex), encryptedFrame)) {
    return false;
  }

  QJsonObject meta;
  meta[QStringLiteral("bufferIndex")] = bufferIndex;
  meta[QStringLiteral("width")] = width;
  meta[QStringLiteral("height")] = height;

  return writeAtomically(SessionPaths::metaFilePath(code_),
                          QJsonDocument(meta).toJson(QJsonDocument::Compact));
}

void SessionWriter::touchHeartbeat() {
  if (!started_) {
    return;
  }
  writeAtomically(SessionPaths::heartbeatFilePath(code_),
                   QByteArray::number(QDateTime::currentMSecsSinceEpoch()));
}

void SessionWriter::stop() {
  if (!started_) {
    return;
  }
  QDir(SessionPaths::sessionDir(code_)).removeRecursively();
  started_ = false;
}

}  // namespace screensharing
