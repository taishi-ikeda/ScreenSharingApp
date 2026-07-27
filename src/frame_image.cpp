#include "screensharing/frame_image.h"

#include <cstring>

namespace screensharing {

namespace {
constexpr int kBytesPerPixel = 3;  // Format_RGB888
}  // namespace

QByteArray packImageBytes(const QImage& image) {
  const QImage rgb = image.convertToFormat(QImage::Format_RGB888);
  const int rowBytes = rgb.width() * kBytesPerPixel;

  QByteArray packed;
  packed.resize(rowBytes * rgb.height());
  for (int y = 0; y < rgb.height(); ++y) {
    std::memcpy(packed.data() + y * rowBytes, rgb.constScanLine(y), rowBytes);
  }
  return packed;
}

QImage unpackImageBytes(const QByteArray& bytes, int width, int height) {
  QImage image(width, height, QImage::Format_RGB888);
  const int rowBytes = width * kBytesPerPixel;
  if (width <= 0 || height <= 0 || bytes.size() < rowBytes * height) {
    image.fill(Qt::black);
    return image;
  }

  for (int y = 0; y < height; ++y) {
    std::memcpy(image.scanLine(y), bytes.constData() + y * rowBytes, rowBytes);
  }
  return image;
}

}  // namespace screensharing
