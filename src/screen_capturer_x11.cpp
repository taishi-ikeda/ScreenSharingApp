#include "screensharing/screen_capturer.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>

#include <cstring>

#include <QElapsedTimer>

namespace screensharing {

namespace {
// How often captureFrame() re-queries XRandR for the tracked output's
// current geometry, so an in-session resolution change (e.g. `xrandr
// --output ... --mode ...`) is picked up without restarting the share.
constexpr qint64 kGeometryRefreshIntervalMs = 2000;

// Looks up outputName's current geometry via XRandR. Returns false if the
// output can't be found or isn't connected (caller keeps the previous
// geometry in that case).
bool queryOutputGeometry(Display* display, Window root, const QString& outputName, int* x, int* y,
                          int* width, int* height) {
  XRRScreenResources* resources = XRRGetScreenResourcesCurrent(display, root);
  if (resources == nullptr) {
    return false;
  }

  bool found = false;
  for (int i = 0; i < resources->noutput && !found; ++i) {
    XRROutputInfo* outputInfo = XRRGetOutputInfo(display, resources, resources->outputs[i]);
    if (outputInfo == nullptr) {
      continue;
    }
    if (outputInfo->connection == RR_Connected && outputInfo->crtc != None &&
        QString::fromLatin1(outputInfo->name, outputInfo->nameLen) == outputName) {
      XRRCrtcInfo* crtcInfo = XRRGetCrtcInfo(display, resources, outputInfo->crtc);
      if (crtcInfo != nullptr) {
        *x = crtcInfo->x;
        *y = crtcInfo->y;
        *width = crtcInfo->width;
        *height = crtcInfo->height;
        found = true;
        XRRFreeCrtcInfo(crtcInfo);
      }
    }
    XRRFreeOutputInfo(outputInfo);
  }

  XRRFreeScreenResources(resources);
  return found;
}

}  // namespace

// X11 capture is synchronous (XGetImage), unlike the async macOS backend:
// captureFrame() grabs a fresh image from the root window on every call.
// Multiple monitors are exposed via XRandR outputs, each a sub-rectangle
// of the single root window (the common case on modern Linux desktops;
// legacy separate-X-screen multi-monitor setups are not handled). The
// tracked output's geometry is periodically re-queried so a mid-session
// resolution change doesn't break the capture region.
// Assumes a 24/32bpp TrueColor visual (the common case on modern desktops);
// unusual depths/visuals are not handled. See SPEC.md section 7.
class X11ScreenCapturer : public ScreenCapturer {
 public:
  X11ScreenCapturer() = default;
  ~X11ScreenCapturer() override { stop(); }

  QList<DisplayInfo> availableDisplays() override;
  bool start(const QString& displayId) override;
  void stop() override;
  QImage captureFrame(double scale) override;
  QSize nativeSize() override;

 private:
  void refreshGeometryIfDue();

  Display* display_ = nullptr;
  Window root_ = 0;
  QString outputName_;  // Empty: capture the whole (multi-monitor) root window.
  int captureX_ = 0;
  int captureY_ = 0;
  int captureWidth_ = 0;
  int captureHeight_ = 0;
  QElapsedTimer geometryRefreshTimer_;
};

QList<DisplayInfo> X11ScreenCapturer::availableDisplays() {
  QList<DisplayInfo> result;

  Display* display = XOpenDisplay(nullptr);
  if (display == nullptr) {
    return result;
  }

  const Window root = RootWindow(display, DefaultScreen(display));
  XRRScreenResources* resources = XRRGetScreenResourcesCurrent(display, root);
  if (resources != nullptr) {
    for (int i = 0; i < resources->noutput; ++i) {
      XRROutputInfo* outputInfo = XRRGetOutputInfo(display, resources, resources->outputs[i]);
      if (outputInfo == nullptr) {
        continue;
      }
      if (outputInfo->connection == RR_Connected && outputInfo->crtc != None) {
        XRRCrtcInfo* crtcInfo = XRRGetCrtcInfo(display, resources, outputInfo->crtc);
        if (crtcInfo != nullptr) {
          const QString name = QString::fromLatin1(outputInfo->name, outputInfo->nameLen);
          DisplayInfo info;
          info.id = name;  // Stable across resolution changes, unlike geometry.
          info.geometry = QRect(crtcInfo->x, crtcInfo->y, crtcInfo->width, crtcInfo->height);
          info.label =
              QStringLiteral("%1 (%2x%3)").arg(name).arg(crtcInfo->width).arg(crtcInfo->height);
          result.append(info);
          XRRFreeCrtcInfo(crtcInfo);
        }
      }
      XRRFreeOutputInfo(outputInfo);
    }
    XRRFreeScreenResources(resources);
  }

  XCloseDisplay(display);
  return result;
}

bool X11ScreenCapturer::start(const QString& displayId) {
  display_ = XOpenDisplay(nullptr);
  if (display_ == nullptr) {
    return false;
  }

  const int screen = DefaultScreen(display_);
  root_ = RootWindow(display_, screen);
  outputName_ = displayId;

  if (outputName_.isEmpty() ||
      !queryOutputGeometry(display_, root_, outputName_, &captureX_, &captureY_, &captureWidth_,
                            &captureHeight_)) {
    // Empty/unrecognized id, or the output vanished: fall back to the
    // whole (possibly multi-monitor) root window.
    outputName_.clear();
    captureX_ = 0;
    captureY_ = 0;
    captureWidth_ = DisplayWidth(display_, screen);
    captureHeight_ = DisplayHeight(display_, screen);
  }
  geometryRefreshTimer_.start();
  return true;
}

void X11ScreenCapturer::stop() {
  if (display_ != nullptr) {
    XCloseDisplay(display_);
    display_ = nullptr;
  }
}

void X11ScreenCapturer::refreshGeometryIfDue() {
  if (outputName_.isEmpty()) {
    return;  // Tracking the whole root window; DisplayWidth/Height below stay current.
  }
  if (geometryRefreshTimer_.isValid() &&
      geometryRefreshTimer_.elapsed() < kGeometryRefreshIntervalMs) {
    return;
  }
  queryOutputGeometry(display_, root_, outputName_, &captureX_, &captureY_, &captureWidth_,
                       &captureHeight_);
  geometryRefreshTimer_.restart();
}

QImage X11ScreenCapturer::captureFrame(double scale) {
  if (display_ == nullptr) {
    return QImage();
  }

  refreshGeometryIfDue();
  if (outputName_.isEmpty()) {
    const int screen = DefaultScreen(display_);
    captureWidth_ = DisplayWidth(display_, screen);
    captureHeight_ = DisplayHeight(display_, screen);
  }

  XImage* xImage = XGetImage(display_, root_, captureX_, captureY_, captureWidth_,
                              captureHeight_, AllPlanes, ZPixmap);
  if (xImage == nullptr) {
    return QImage();
  }

  // XGetImage on a TrueColor 24/32bpp visual returns 4 bytes/pixel in
  // host byte order, which matches QImage::Format_RGB32 (0xffRRGGBB,
  // byte order BB,GG,RR,xx on this little-endian platform).
  QImage frame(xImage->width, xImage->height, QImage::Format_RGB32);
  for (int y = 0; y < xImage->height; ++y) {
    std::memcpy(frame.scanLine(y), xImage->data + y * xImage->bytes_per_line,
                static_cast<size_t>(xImage->width) * 4);
  }
  XDestroyImage(xImage);

  if (scale >= 1.0) {
    return frame;
  }
  // Scale is uniform (same factor on both axes), so this can never distort
  // the aspect ratio -- unlike scaling to some externally-chosen bounding
  // box. It's relative to frame's actual just-captured size, so a
  // mid-session resolution change is reflected automatically.
  const QSize targetSize(static_cast<int>(frame.width() * scale + 0.5),
                          static_cast<int>(frame.height() * scale + 0.5));
  return frame.scaled(targetSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

QSize X11ScreenCapturer::nativeSize() { return QSize(captureWidth_, captureHeight_); }

std::unique_ptr<ScreenCapturer> ScreenCapturer::create() {
  return std::make_unique<X11ScreenCapturer>();
}

}  // namespace screensharing
