#include <QtTest/QtTest>

#include "screensharing/xor_cipher.h"

namespace screensharing {
namespace {

class XorCipherTest : public QObject {
  Q_OBJECT

 private slots:
  void transformTwiceRestoresOriginalData();
  void outputLengthMatchesInputLength();
  void differentCodesProduceDifferentCiphertext();
  void keystreamDoesNotRepeatAcrossBlockBoundary();
};

void XorCipherTest::transformTwiceRestoresOriginalData() {
  const XorCipher cipher(QStringLiteral("1234"));
  const QByteArray original = QByteArrayLiteral("the quick brown fox jumps over the lazy dog");

  const QByteArray encrypted = cipher.transform(original);
  const QByteArray decrypted = cipher.transform(encrypted);

  QCOMPARE(decrypted, original);
  QVERIFY(encrypted != original);
}

void XorCipherTest::outputLengthMatchesInputLength() {
  const XorCipher cipher(QStringLiteral("5678"));
  const QByteArray data(100, 'x');
  QCOMPARE(cipher.transform(data).size(), data.size());
}

void XorCipherTest::differentCodesProduceDifferentCiphertext() {
  const QByteArray data = QByteArrayLiteral("identical plaintext");
  const QByteArray encryptedA = XorCipher(QStringLiteral("0001")).transform(data);
  const QByteArray encryptedB = XorCipher(QStringLiteral("0002")).transform(data);
  QVERIFY(encryptedA != encryptedB);
}

void XorCipherTest::keystreamDoesNotRepeatAcrossBlockBoundary() {
  // SHA-256 blocks are 32 bytes; use 64 zero bytes so the keystream itself
  // (XOR with zero == keystream) spans two blocks, and check they differ.
  const XorCipher cipher(QStringLiteral("9999"));
  const QByteArray zeros(64, '\0');
  const QByteArray keystream = cipher.transform(zeros);

  QCOMPARE(keystream.size(), 64);
  QVERIFY(keystream.left(32) != keystream.mid(32, 32));
}

}  // namespace
}  // namespace screensharing

QTEST_MAIN(screensharing::XorCipherTest)
#include "test_xor_cipher.moc"
