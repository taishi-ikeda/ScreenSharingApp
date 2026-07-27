#include "screensharing/presenter_panel.h"

#include <QComboBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

#include "screensharing/frame_image.h"
#include "screensharing/invite_code.h"
#include "screensharing/log_dialog.h"
#include "screensharing/logger.h"
#include "screensharing/screen_capturer.h"
#include "screensharing/session_writer.h"
#include "screensharing/xor_cipher.h"

namespace screensharing {

namespace {
constexpr int kFrameIntervalMs = 1000 / 15;  // SPEC.md default: 15fps.
}  // namespace

PresenterPanel::PresenterPanel(QWidget* parent)
    : QWidget(parent),
      inviteCodeLabel_(new QLabel(tr("----"), this)),
      displayComboBox_(new QComboBox(this)),
      resolutionComboBox_(new QComboBox(this)),
      statsLabel_(new QLabel(tr("fps: -- / throughput: --"), this)),
      startButton_(new QPushButton(tr("Start"), this)),
      stopButton_(new QPushButton(tr("Stop"), this)),
      logButton_(new QPushButton(tr("View Log"), this)),
      backButton_(new QPushButton(tr("Back"), this)),
      logger_(new Logger(this)),
      logDialog_(nullptr),
      frameTimer_(new QTimer(this)),
      framesInWindow_(0),
      bytesInWindow_(0),
      capturer_(ScreenCapturer::create()) {
  inviteCodeLabel_->setObjectName(QStringLiteral("inviteCodeLabel"));
  displays_ = capturer_->availableDisplays();
  for (const DisplayInfo& display : displays_) {
    displayComboBox_->addItem(display.label, display.id);
  }
  updateResolutionOptions();
  stopButton_->setEnabled(false);

  auto* navRow = new QHBoxLayout;
  navRow->addWidget(backButton_, 0, Qt::AlignLeft);
  navRow->addStretch();
  navRow->addWidget(logButton_, 0, Qt::AlignRight);

  auto* buttonRow = new QHBoxLayout;
  buttonRow->addWidget(startButton_);
  buttonRow->addWidget(stopButton_);

  auto* inviteCodeRow = new QHBoxLayout;
  inviteCodeRow->addWidget(new QLabel(tr("Invite Code:"), this));
  inviteCodeRow->addWidget(inviteCodeLabel_);
  inviteCodeRow->addStretch();

  auto* displayRow = new QHBoxLayout;
  displayRow->addWidget(new QLabel(tr("Display:"), this));
  displayRow->addWidget(displayComboBox_);

  auto* resolutionRow = new QHBoxLayout;
  resolutionRow->addWidget(new QLabel(tr("Resolution:"), this));
  resolutionRow->addWidget(resolutionComboBox_);

  auto* layout = new QVBoxLayout(this);
  layout->addLayout(navRow);
  layout->addLayout(inviteCodeRow);
  layout->addLayout(displayRow);
  layout->addLayout(resolutionRow);
  layout->addWidget(statsLabel_);
  layout->addLayout(buttonRow);
  layout->addStretch();

  frameTimer_->setInterval(kFrameIntervalMs);
  connect(frameTimer_, &QTimer::timeout, this, &PresenterPanel::publishNextFrame);

  connect(displayComboBox_, QOverload<int>::of(&QComboBox::activated), this,
          &PresenterPanel::onDisplaySelectionChanged);

  connect(startButton_, &QPushButton::clicked, this,
          &PresenterPanel::onStartClicked);
  connect(stopButton_, &QPushButton::clicked, this,
          &PresenterPanel::onStopClicked);
  connect(logButton_, &QPushButton::clicked, this,
          &PresenterPanel::onLogClicked);
  connect(backButton_, &QPushButton::clicked, this, [this]() {
    if (writer_) {
      onStopClicked();
    }
    emit backRequested();
  });

  generateInviteCode();
}

PresenterPanel::~PresenterPanel() = default;

void PresenterPanel::generateInviteCode() {
  inviteCode_ = generateAvailableInviteCode();
  if (inviteCode_.isEmpty()) {
    inviteCodeLabel_->setText(tr("(none available)"));
    logger_->log(tr("No invite code available"));
  } else {
    inviteCodeLabel_->setText(inviteCode_);
    logger_->log(tr("Invite code generated: %1").arg(inviteCode_));
  }
}

QSize PresenterPanel::selectedResolution() const {
  const QStringList parts =
      resolutionComboBox_->currentText().split(QLatin1Char('x'));
  if (parts.size() != 2) {
    return QSize(1280, 720);
  }
  return QSize(parts.at(0).toInt(), parts.at(1).toInt());
}

QString PresenterPanel::selectedDisplayId() const {
  return displayComboBox_->currentData().toString();
}

void PresenterPanel::onDisplaySelectionChanged(int index) {
  updateResolutionOptions();
  if (index < 0 || index >= displays_.size()) {
    return;
  }
  showDisplayIdentifier(displays_.at(index));
}

void PresenterPanel::updateResolutionOptions() {
  double aspectRatio = 16.0 / 9.0;
  const int index = displayComboBox_->currentIndex();
  if (index >= 0 && index < displays_.size()) {
    const QRect& geometry = displays_.at(index).geometry;
    if (geometry.width() > 0 && geometry.height() > 0) {
      aspectRatio = static_cast<double>(geometry.width()) / geometry.height();
    }
  }

  resolutionComboBox_->clear();
  for (const int height : {1080, 720, 480}) {
    int width = static_cast<int>(height * aspectRatio + 0.5);
    width -= width % 2;  // keep dimensions even.
    resolutionComboBox_->addItem(tr("%1x%2").arg(width).arg(height));
  }
}

void PresenterPanel::showDisplayIdentifier(const DisplayInfo& display) {
  QScreen* targetScreen = nullptr;
  for (QScreen* screen : QGuiApplication::screens()) {
    if (screen->geometry() == display.geometry) {
      targetScreen = screen;
      break;
    }
  }
  if (targetScreen == nullptr) {
    for (QScreen* screen : QGuiApplication::screens()) {
      if (screen->geometry().contains(display.geometry.topLeft())) {
        targetScreen = screen;
        break;
      }
    }
  }
  if (targetScreen == nullptr) {
    return;
  }

  auto* overlay = new QLabel(display.label);
  overlay->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
  overlay->setAttribute(Qt::WA_ShowWithoutActivating);
  overlay->setAttribute(Qt::WA_DeleteOnClose);
  overlay->setAlignment(Qt::AlignCenter);
  overlay->setWordWrap(true);
  overlay->setStyleSheet(QStringLiteral(
      "background-color: rgba(0, 0, 0, 200); color: white; font-size: 72px; font-weight: bold;"));
  overlay->setGeometry(targetScreen->geometry());
  overlay->show();

  QTimer::singleShot(1500, overlay, &QWidget::close);
}

void PresenterPanel::onStartClicked() {
  if (inviteCode_.isEmpty()) {
    return;
  }

  if (!capturer_->start(selectedDisplayId())) {
    statsLabel_->setText(tr("Failed to start screen capture"));
    logger_->log(tr("Failed to start screen capture"));
    return;
  }

  writer_ = std::make_unique<SessionWriter>(inviteCode_);
  if (!writer_->start()) {
    statsLabel_->setText(tr("Failed to start session"));
    logger_->log(tr("Failed to start session (code: %1)").arg(inviteCode_));
    writer_.reset();
    capturer_->stop();
    return;
  }

  logger_->log(tr("Session started (code: %1)").arg(inviteCode_));

  framesInWindow_ = 0;
  bytesInWindow_ = 0;
  statsTimer_.start();

  startButton_->setEnabled(false);
  stopButton_->setEnabled(true);
  displayComboBox_->setEnabled(false);
  resolutionComboBox_->setEnabled(false);
  frameTimer_->start();
}

void PresenterPanel::onStopClicked() {
  const bool wasRunning = static_cast<bool>(writer_);

  frameTimer_->stop();
  writer_.reset();
  capturer_->stop();

  if (wasRunning) {
    logger_->log(tr("Session stopped (code: %1)").arg(inviteCode_));
  }

  startButton_->setEnabled(true);
  stopButton_->setEnabled(false);
  displayComboBox_->setEnabled(true);
  resolutionComboBox_->setEnabled(true);
  statsLabel_->setText(tr("fps: -- / throughput: --"));
}

void PresenterPanel::publishNextFrame() {
  if (!writer_) {
    return;
  }

  const QSize resolution = selectedResolution();
  const QImage frame = capturer_->captureFrame(resolution);
  if (frame.isNull()) {
    // Capture hasn't produced a frame yet (e.g. still warming up, or the
    // OS permission prompt hasn't been answered). Keep the session alive
    // and try again on the next tick.
    writer_->touchHeartbeat();
    return;
  }

  const QByteArray raw = packImageBytes(frame);
  const QByteArray encrypted = XorCipher(inviteCode_).transform(raw);

  // frame's actual size, not the requested bounding box: captureFrame()
  // preserves the source display's aspect ratio, so it may be smaller
  // than `resolution` in one dimension.
  if (writer_->writeFrame(encrypted, frame.width(), frame.height())) {
    writer_->touchHeartbeat();
    ++framesInWindow_;
    bytesInWindow_ += encrypted.size();
  }

  const qint64 elapsedMs = statsTimer_.elapsed();
  if (elapsedMs >= 1000) {
    const double seconds = elapsedMs / 1000.0;
    const double fps = framesInWindow_ / seconds;
    const double throughputMBps = (bytesInWindow_ / seconds) / (1024.0 * 1024.0);
    statsLabel_->setText(tr("fps: %1 / throughput: %2 MB/s")
                              .arg(fps, 0, 'f', 1)
                              .arg(throughputMBps, 0, 'f', 2));
    framesInWindow_ = 0;
    bytesInWindow_ = 0;
    statsTimer_.restart();
  }
}

void PresenterPanel::onLogClicked() {
  if (logDialog_ == nullptr) {
    logDialog_ = new LogDialog(logger_, this);
  }
  logDialog_->show();
  logDialog_->raise();
  logDialog_->activateWindow();
}

}  // namespace screensharing
