#include "screensharing/xor_cipher.h"

#include <QCryptographicHash>
#include <QDataStream>
#include <QIODevice>

namespace screensharing {

namespace {
constexpr int kBlockSize = 32;  // SHA-256 digest size.
}  // namespace

XorCipher::XorCipher(const QString& code)
    : seed_(QCryptographicHash::hash(code.toUtf8(), QCryptographicHash::Sha256)) {}

QByteArray XorCipher::keystreamBlock(quint32 blockIndex) const {
  QByteArray input = seed_;
  QDataStream stream(&input, QIODevice::WriteOnly | QIODevice::Append);
  stream.setByteOrder(QDataStream::BigEndian);
  stream << blockIndex;
  return QCryptographicHash::hash(input, QCryptographicHash::Sha256);
}

QByteArray XorCipher::transform(const QByteArray& data) const {
  QByteArray result;
  result.resize(data.size());

  quint32 blockIndex = 0;
  QByteArray block = keystreamBlock(blockIndex);
  for (int i = 0; i < data.size(); ++i) {
    const int offsetInBlock = i % kBlockSize;
    if (i > 0 && offsetInBlock == 0) {
      block = keystreamBlock(++blockIndex);
    }
    result[i] = data[i] ^ block[offsetInBlock];
  }
  return result;
}

}  // namespace screensharing
