#include "screensharing/screen_capturer.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>

#include <cstring>

namespace screensharing {

// X11 capture is synchronous (XGetImage), unlike the async macOS backend:
// captureFrame() grabs a fresh image from the root window on every call.
// Multiple monitors are exposed via XRandR outputs, each a sub-rectangle
// of the single root window (the common case on modern Linux desktops;
// legacy separate-X-screen multi-monitor setups are not handled).
// Assumes a 24/32bpp TrueColor visual (the common case on modern desktops);
// unusual depths/visuals are not handled. See SPEC.md section 7.
class X11ScreenCapturer : public ScreenCapturer {
 public:
  X11ScreenCapturer() = default;
  ~X11ScreenCapturer() override { stop(); }

  QList<DisplayInfo> availableDisplays() override;
  bool start(const QString& displayId) override;
  void stop() override;
  QImage captureFrame(const QSize& targetSize) override;

 private:
  Display* display_ = nullptr;
  Window root_ = 0;
  int captureX_ = 0;
  int captureY_ = 0;
  int captureWidth_ = 0;
  int captureHeight_ = 0;
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
          DisplayInfo info;
          info.id = QStringLiteral("%1,%2,%3,%4")
                         .arg(crtcInfo->x)
                         .arg(crtcInfo->y)
                         .arg(crtcInfo->width)
                         .arg(crtcInfo->height);
          info.geometry = QRect(crtcInfo->x, crtcInfo->y, crtcInfo->width, crtcInfo->height);
          info.label = QStringLiteral("%1 (%2x%3)")
                           .arg(QString::fromLatin1(outputInfo->name, outputInfo->nameLen))
                           .arg(crtcInfo->width)
                           .arg(crtcInfo->height);
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

  const QStringList parts = displayId.split(QLatin1Char(','));
  if (parts.size() == 4) {
    captureX_ = parts.at(0).toInt();
    captureY_ = parts.at(1).toInt();
    captureWidth_ = parts.at(2).toInt();
    captureHeight_ = parts.at(3).toInt();
  } else {
    // Empty/unrecognized id: fall back to the whole (possibly
    // multi-monitor) root window.
    captureX_ = 0;
    captureY_ = 0;
    captureWidth_ = DisplayWidth(display_, screen);
    captureHeight_ = DisplayHeight(display_, screen);
  }
  return true;
}

void X11ScreenCapturer::stop() {
  if (display_ != nullptr) {
    XCloseDisplay(display_);
    display_ = nullptr;
  }
}

QImage X11ScreenCapturer::captureFrame(const QSize& targetSize) {
  if (display_ == nullptr) {
    return QImage();
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

  // KeepAspectRatio: targetSize is a bounding box, not a forced shape.
  // Distorting the aspect ratio here would bake it into the image before
  // it's even sent; the actual output size may be smaller than targetSize
  // in one dimension. The caller must use the returned image's own size,
  // not targetSize, when publishing it.
  return frame.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

std::unique_ptr<ScreenCapturer> ScreenCapturer::create() {
  return std::make_unique<X11ScreenCapturer>();
}

}  // namespace screensharing
