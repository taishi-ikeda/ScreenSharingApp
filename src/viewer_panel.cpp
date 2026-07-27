#include "screensharing/viewer_panel.h"

#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include "screensharing/frame_image.h"
#include "screensharing/log_dialog.h"
#include "screensharing/logger.h"
#include "screensharing/session_reader.h"
#include "screensharing/xor_cipher.h"

namespace screensharing {

namespace {
constexpr int kPollIntervalMs = 1000 / 15;  // Matches the presenter's default fps.
}  // namespace

ViewerPanel::ViewerPanel(QWidget* parent)
    : QWidget(parent),
      codeLineEdit_(new QLineEdit(this)),
      connectButton_(new QPushButton(tr("Connect"), this)),
      statusLabel_(new QLabel(tr("Not connected"), this)),
      videoLabel_(new QLabel(this)),
      logButton_(new QPushButton(tr("View Log"), this)),
      backButton_(new QPushButton(tr("Back"), this)),
      logger_(new Logger(this)),
      logDialog_(nullptr),
      pollTimer_(new QTimer(this)) {
  codeLineEdit_->setObjectName(QStringLiteral("codeLineEdit"));
  statusLabel_->setObjectName(QStringLiteral("statusLabel"));
  codeLineEdit_->setMaxLength(4);
  codeLineEdit_->setPlaceholderText(tr("4-digit invite code"));
  videoLabel_->setMinimumSize(320, 180);
  videoLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  videoLabel_->setAlignment(Qt::AlignCenter);
  videoLabel_->setStyleSheet(QStringLiteral("background-color: black;"));

  auto* navRow = new QHBoxLayout;
  navRow->addWidget(backButton_, 0, Qt::AlignLeft);
  navRow->addStretch();
  navRow->addWidget(logButton_, 0, Qt::AlignRight);

  auto* codeRow = new QHBoxLayout;
  codeRow->addWidget(codeLineEdit_);
  codeRow->addWidget(connectButton_);

  auto* layout = new QVBoxLayout(this);
  layout->addLayout(navRow);
  layout->addLayout(codeRow);
  layout->addWidget(statusLabel_);
  layout->addWidget(videoLabel_, /*stretch=*/1);

  pollTimer_->setInterval(kPollIntervalMs);
  connect(pollTimer_, &QTimer::timeout, this, &ViewerPanel::pollForFrame);

  connect(connectButton_, &QPushButton::clicked, this,
          &ViewerPanel::onConnectClicked);
  connect(logButton_, &QPushButton::clicked, this,
          &ViewerPanel::onLogClicked);
  connect(backButton_, &QPushButton::clicked, this, [this]() {
    if (reader_) {
      logger_->log(tr("Left session %1").arg(connectedCode_));
    }
    resetConnection();
    statusLabel_->setText(tr("Not connected"));
    emit backRequested();
  });
}

ViewerPanel::~ViewerPanel() = default;

void ViewerPanel::onConnectClicked() {
  const QString code = codeLineEdit_->text();
  if (code.size() != 4) {
    statusLabel_->setText(tr("Enter a 4-digit code"));
    logger_->log(tr("Invalid invite code entered"));
    return;
  }

  auto candidate = std::make_unique<SessionReader>(code);
  if (!candidate->isAlive()) {
    statusLabel_->setText(tr("No active session for code %1").arg(code));
    logger_->log(tr("No active session for code %1").arg(code));
    return;
  }

  reader_ = std::move(candidate);
  connectedCode_ = code;
  statusLabel_->setText(tr("Connected: %1").arg(code));
  logger_->log(tr("Connected to session %1").arg(code));
  pollTimer_->start();
}

void ViewerPanel::pollForFrame() {
  if (!reader_) {
    return;
  }
  if (!reader_->isAlive()) {
    disconnectSession(tr("Disconnected: %1").arg(connectedCode_));
    return;
  }

  QByteArray encrypted;
  int width = 0;
  int height = 0;
  if (!reader_->readFrame(&encrypted, &width, &height) || width <= 0 || height <= 0) {
    return;
  }

  const QByteArray raw = XorCipher(connectedCode_).transform(encrypted);
  updateVideoFrame(unpackImageBytes(raw, width, height));
}

void ViewerPanel::updateVideoFrame(const QImage& image) {
  videoLabel_->setPixmap(QPixmap::fromImage(image).scaled(
      videoLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void ViewerPanel::resetConnection() {
  pollTimer_->stop();
  reader_.reset();
  videoLabel_->setPixmap(QPixmap());
}

void ViewerPanel::disconnectSession(const QString& reason) {
  if (reader_) {
    logger_->log(reason);
  }
  resetConnection();
  statusLabel_->setText(reason);
}

void ViewerPanel::onLogClicked() {
  if (logDialog_ == nullptr) {
    logDialog_ = new LogDialog(logger_, this);
  }
  logDialog_->show();
  logDialog_->raise();
  logDialog_->activateWindow();
}

}  // namespace screensharing
