#pragma once

#include <QByteArray>
#include <QImage>

namespace screensharing {

// Packs a QImage into a tightly-packed Format_RGB888 byte buffer with no
// per-row padding, matching the "raw bitmap" wire format in SPEC.md
// section 7 (QImage's own scanlines may be padded to a 4-byte boundary).
QByteArray packImageBytes(const QImage& image);

// Inverse of packImageBytes: reconstructs a Format_RGB888 QImage of the
// given size from a tightly-packed byte buffer.
QImage unpackImageBytes(const QByteArray& bytes, int width, int height);

}  // namespace screensharing
