#include "screensharing/screen_capturer.h"

#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

#include <atomic>
#include <cstring>

#include <QCoreApplication>
#include <QMutex>
#include <QMutexLocker>

namespace screensharing {
class MacScreenCapturer;
}  // namespace screensharing

API_AVAILABLE(macos(12.3))
@interface ScreenSharingStreamOutput : NSObject <SCStreamOutput>
- (instancetype)initWithCapturer:(screensharing::MacScreenCapturer*)capturer;
@end

namespace screensharing {

// ScreenCaptureKit delivers frames asynchronously via a delegate callback on
// a background dispatch queue; captureFrame() just hands back whatever the
// most recent callback stored. See SPEC.md section 7.
class MacScreenCapturer : public ScreenCapturer {
 public:
  MacScreenCapturer() = default;
  ~MacScreenCapturer() override { stop(); }

  QList<DisplayInfo> availableDisplays() override;
  bool start(const QString& displayId) override;
  void stop() override;
  QImage captureFrame(const QSize& targetSize) override;

  void storeFrame(const QImage& image);

 private:
  QMutex mutex_;
  SCStream* stream_ = nil;
  ScreenSharingStreamOutput* output_ = nil;
  dispatch_queue_t sampleQueue_ = nil;
  std::atomic<bool> cancelled_{false};
  QImage latestFrame_;
};

namespace {

// NSScreen.backingScaleFactor is per-display (e.g. Retina internal vs a
// non-Retina external monitor), so we look up the screen matching the
// target CGDirectDisplayID rather than assuming the main screen's scale.
CGFloat backingScaleFactorForDisplay(CGDirectDisplayID displayId) {
  for (NSScreen* screen in [NSScreen screens]) {
    NSNumber* screenNumber = screen.deviceDescription[@"NSScreenNumber"];
    if (screenNumber != nil && [screenNumber unsignedIntValue] == displayId) {
      return screen.backingScaleFactor;
    }
  }
  return [NSScreen mainScreen].backingScaleFactor;
}

}  // namespace

QList<DisplayInfo> MacScreenCapturer::availableDisplays() {
  QList<DisplayInfo> result;

  CGDirectDisplayID displayIds[16];
  uint32_t count = 0;
  if (CGGetActiveDisplayList(16, displayIds, &count) != kCGErrorSuccess) {
    return result;
  }

  for (uint32_t i = 0; i < count; ++i) {
    const CGDirectDisplayID displayId = displayIds[i];
    const int width = static_cast<int>(CGDisplayPixelsWide(displayId));
    const int height = static_cast<int>(CGDisplayPixelsHigh(displayId));
    const CGRect bounds = CGDisplayBounds(displayId);

    DisplayInfo info;
    info.id = QString::number(displayId);
    info.geometry = QRect(static_cast<int>(bounds.origin.x), static_cast<int>(bounds.origin.y),
                           static_cast<int>(bounds.size.width),
                           static_cast<int>(bounds.size.height));
    info.label = (CGDisplayIsMain(displayId)
                      ? QObject::tr("Display %1 (Main, %2x%3)")
                      : QObject::tr("Display %1 (%2x%3)"))
                     .arg(i + 1)
                     .arg(width)
                     .arg(height);
    result.append(info);
  }
  return result;
}

bool MacScreenCapturer::start(const QString& displayId) {
  cancelled_ = false;
  const CGDirectDisplayID targetId =
      displayId.isEmpty() ? kCGDirectMainDisplay
                           : static_cast<CGDirectDisplayID>(displayId.toUInt());

  [SCShareableContent getShareableContentWithCompletionHandler:^(
                           SCShareableContent* content, NSError* error) {
    if (cancelled_ || error != nil || content.displays.count == 0) {
      if (error != nil) {
        NSLog(@"ScreenSharing: failed to enumerate displays: %@", error);
      }
      return;
    }

    SCDisplay* display = nil;
    for (SCDisplay* candidate in content.displays) {
      if (candidate.displayID == targetId) {
        display = candidate;
        break;
      }
    }
    if (display == nil) {
      display = content.displays.firstObject;
    }

    SCContentFilter* filter =
        [[SCContentFilter alloc] initWithDisplay:display excludingWindows:@[]];

    SCStreamConfiguration* config = [[SCStreamConfiguration alloc] init];
    const CGFloat scale = backingScaleFactorForDisplay(display.displayID);
    config.width = static_cast<NSInteger>(display.width * scale);
    config.height = static_cast<NSInteger>(display.height * scale);
    config.pixelFormat = kCVPixelFormatType_32BGRA;
    config.minimumFrameInterval = CMTimeMake(1, 15);
    config.queueDepth = 3;

    QMutexLocker locker(&mutex_);
    if (cancelled_) {
      return;
    }

    stream_ = [[SCStream alloc] initWithFilter:filter configuration:config delegate:nil];
    output_ = [[ScreenSharingStreamOutput alloc] initWithCapturer:this];
    sampleQueue_ = dispatch_queue_create("com.screensharing.capture", DISPATCH_QUEUE_SERIAL);

    NSError* addError = nil;
    [stream_ addStreamOutput:output_
                         type:SCStreamOutputTypeScreen
           sampleHandlerQueue:sampleQueue_
                        error:&addError];
    if (addError != nil) {
      NSLog(@"ScreenSharing: addStreamOutput failed: %@", addError);
      return;
    }

    [stream_ startCaptureWithCompletionHandler:^(NSError* captureError) {
      if (captureError != nil) {
        NSLog(@"ScreenSharing: startCapture failed: %@", captureError);
      }
    }];
  }];

  // ScreenCaptureKit's setup is asynchronous (and may involve the OS
  // permission prompt), so we can't synchronously report success/failure
  // here without risking a main-thread deadlock. captureFrame() simply
  // returns a null image until the first frame arrives.
  return true;
}

void MacScreenCapturer::stop() {
  cancelled_ = true;

  QMutexLocker locker(&mutex_);
  if (stream_ != nil) {
    [stream_ stopCaptureWithCompletionHandler:^(NSError*) {
    }];
    stream_ = nil;
  }
  output_ = nil;
  sampleQueue_ = nil;
  latestFrame_ = QImage();
}

QImage MacScreenCapturer::captureFrame(const QSize& targetSize) {
  QImage snapshot;
  {
    QMutexLocker locker(&mutex_);
    snapshot = latestFrame_;
  }
  if (snapshot.isNull()) {
    return QImage();
  }
  // KeepAspectRatio: targetSize is a bounding box, not a forced shape.
  // Distorting the aspect ratio here would bake it into the image before
  // it's even sent; the actual output size may be smaller than targetSize
  // in one dimension. The caller must use the returned image's own size,
  // not targetSize, when publishing it.
  return snapshot.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void MacScreenCapturer::storeFrame(const QImage& image) {
  QMutexLocker locker(&mutex_);
  latestFrame_ = image;
}

}  // namespace screensharing

@implementation ScreenSharingStreamOutput {
  screensharing::MacScreenCapturer* _capturer;
}

- (instancetype)initWithCapturer:(screensharing::MacScreenCapturer*)capturer {
  self = [super init];
  if (self) {
    _capturer = capturer;
  }
  return self;
}

- (void)stream:(SCStream*)stream
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
                    ofType:(SCStreamOutputType)type {
  if (type != SCStreamOutputTypeScreen || !CMSampleBufferIsValid(sampleBuffer)) {
    return;
  }
  CVImageBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
  if (pixelBuffer == nullptr) {
    return;
  }

  CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
  const int width = static_cast<int>(CVPixelBufferGetWidth(pixelBuffer));
  const int height = static_cast<int>(CVPixelBufferGetHeight(pixelBuffer));
  const uint8_t* base = static_cast<const uint8_t*>(CVPixelBufferGetBaseAddress(pixelBuffer));
  const size_t bytesPerRow = CVPixelBufferGetBytesPerRow(pixelBuffer);

  // ScreenCaptureKit delivers 32BGRA, which is byte-identical in memory to
  // QImage::Format_ARGB32 on this (little-endian) platform.
  QImage frame(width, height, QImage::Format_ARGB32);
  for (int y = 0; y < height; ++y) {
    std::memcpy(frame.scanLine(y), base + y * bytesPerRow, static_cast<size_t>(width) * 4);
  }
  CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

  _capturer->storeFrame(frame);
}

@end

namespace screensharing {

std::unique_ptr<ScreenCapturer> ScreenCapturer::create() {
  return std::make_unique<MacScreenCapturer>();
}

}  // namespace screensharing
