#pragma once

#include <QByteArray>
#include <QString>

namespace screensharing {

// Owns one presenter-side session directory under the shared base dir:
// creates it, writes frames (double-buffered, atomic via rename), updates
// the heartbeat, and removes the directory again when the session ends.
// Frame/metadata encryption is layered on top by the caller; this class
// only deals with already-encrypted bytes.
class SessionWriter {
 public:
  explicit SessionWriter(const QString& code);
  ~SessionWriter();

  SessionWriter(const SessionWriter&) = delete;
  SessionWriter& operator=(const SessionWriter&) = delete;

  // Creates the session directory and writes the initial heartbeat.
  // Returns false on I/O failure.
  bool start();

  // Writes one already-encrypted frame plus its metadata, atomically.
  bool writeFrame(const QByteArray& encryptedFrame, int width, int height);

  // Refreshes the heartbeat file; call this periodically while sharing.
  void touchHeartbeat();

  // Removes the session directory. Safe to call multiple times.
  void stop();

 private:
  QString code_;
  int nextBufferIndex_;
  bool started_;
};

}  // namespace screensharing
