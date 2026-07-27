#pragma once

#include <QByteArray>
#include <QString>

namespace screensharing {

// A simple stream cipher keyed by the 4-digit invite code. This deters
// casual snooping by other accounts on the same machine; it is not meant
// to withstand a determined attacker (4 digits = 10000 possible keys).
// The keystream is built from QCryptographicHash (SHA-256), which is part
// of QtCore and produces identical output regardless of Qt version, so a
// Qt5-built and Qt6-built instance can interoperate. See SPEC.md section 6.
class XorCipher {
 public:
  explicit XorCipher(const QString& code);

  // XOR is its own inverse, so the same function encrypts and decrypts.
  QByteArray transform(const QByteArray& data) const;

 private:
  QByteArray keystreamBlock(quint32 blockIndex) const;

  QByteArray seed_;
};

}  // namespace screensharing
