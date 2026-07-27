#pragma once

#include <QImage>
#include <QList>
#include <QRect>
#include <QSize>
#include <QString>

#include <memory>

namespace screensharing {

// Describes one capturable display for a UI picker.
struct DisplayInfo {
  QString id;     // Opaque, platform-specific; pass back to start().
  QString label;  // Human-readable text for a combo box.
  QRect geometry; // Position + size in the desktop's global coordinate
                  // space (matches QScreen::geometry()), used to show an
                  // on-screen identifier overlay on the right monitor.
};

// Captures a display. Platform backends may be asynchronous (macOS
// ScreenCaptureKit delivers frames via a background callback), so this
// interface is pull-based: start() begins capture and returns immediately,
// and captureFrame() always returns whatever the most recently captured
// frame is, scaled to the requested size. Returns a null QImage if nothing
// has been captured yet (e.g. capture just started, or the OS denied
// permission). See SPEC.md section 7.
class ScreenCapturer {
 public:
  virtual ~ScreenCapturer() = default;

  // Enumerates displays available to capture. Safe to call at any time,
  // including before start().
  virtual QList<DisplayInfo> availableDisplays() = 0;

  // Starts capturing the display identified by displayId (an id returned
  // by availableDisplays()). An empty or unrecognized id falls back to the
  // primary/whole-screen display.
  virtual bool start(const QString& displayId) = 0;
  virtual void stop() = 0;
  virtual QImage captureFrame(const QSize& targetSize) = 0;

  // Creates the capturer for the current platform (macOS: ScreenCaptureKit,
  // Linux: X11).
  static std::unique_ptr<ScreenCapturer> create();
};

}  // namespace screensharing
