#pragma once

#include <QByteArray>
#include <QString>

namespace screensharing {

// Reads frames from a presenter's session directory under the shared base
// dir, and reports whether the session is still alive via its heartbeat.
// Decryption is layered on top by the caller; this class only deals with
// the encrypted bytes on disk.
class SessionReader {
 public:
  explicit SessionReader(const QString& code);

  // True if the session directory has a fresh-enough heartbeat.
  bool isAlive() const;

  // Reads the current frame and its declared width/height from meta.json.
  // Returns false if no frame has been published yet or a read fails.
  bool readFrame(QByteArray* encryptedFrame, int* width, int* height) const;

 private:
  QString code_;
};

}  // namespace screensharing
