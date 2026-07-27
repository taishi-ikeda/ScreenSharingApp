#include "screensharing/session_reader.h"

#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "screensharing/session_paths.h"

namespace screensharing {

namespace {
constexpr qint64 kHeartbeatStaleMs = 3000;
}  // namespace

SessionReader::SessionReader(const QString& code) : code_(code) {}

bool SessionReader::isAlive() const {
  QFile heartbeatFile(SessionPaths::heartbeatFilePath(code_));
  if (!heartbeatFile.open(QIODevice::ReadOnly)) {
    return false;
  }
  bool ok = false;
  const qint64 lastBeatMs = heartbeatFile.readAll().toLongLong(&ok);
  if (!ok) {
    return false;
  }
  return (QDateTime::currentMSecsSinceEpoch() - lastBeatMs) < kHeartbeatStaleMs;
}

bool SessionReader::readFrame(QByteArray* encryptedFrame, int* width, int* height) const {
  QFile metaFile(SessionPaths::metaFilePath(code_));
  if (!metaFile.open(QIODevice::ReadOnly)) {
    return false;
  }
  const QJsonObject meta = QJsonDocument::fromJson(metaFile.readAll()).object();
  const int bufferIndex = meta[QStringLiteral("bufferIndex")].toInt(-1);
  if (bufferIndex != 0 && bufferIndex != 1) {
    return false;
  }

  QFile frameFile(SessionPaths::frameFilePath(code_, bufferIndex));
  if (!frameFile.open(QIODevice::ReadOnly)) {
    return false;
  }

  *encryptedFrame = frameFile.readAll();
  *width = meta[QStringLiteral("width")].toInt();
  *height = meta[QStringLiteral("height")].toInt();
  return true;
}

}  // namespace screensharing
