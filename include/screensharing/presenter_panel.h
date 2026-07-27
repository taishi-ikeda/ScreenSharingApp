#pragma once

#include <QElapsedTimer>
#include <QSize>
#include <QString>
#include <QWidget>

#include <memory>

#include "screensharing/screen_capturer.h"

class QComboBox;
class QLabel;
class QPushButton;
class QTimer;

namespace screensharing {

class LogDialog;
class Logger;
class SessionWriter;

// UI panel for the "presenter" (screen-sharing) role.
// Captures the screen via ScreenCapturer and publishes frames through
// SessionWriter.
class PresenterPanel : public QWidget {
  Q_OBJECT

 public:
  explicit PresenterPanel(QWidget* parent = nullptr);
  ~PresenterPanel() override;

 signals:
  void backRequested();

 private slots:
  void onStartClicked();
  void onStopClicked();
  void onLogClicked();
  void publishNextFrame();
  void onDisplaySelectionChanged(int index);

 private:
  void generateInviteCode();
  QSize selectedResolution() const;
  QString selectedDisplayId() const;
  void showDisplayIdentifier(const DisplayInfo& display);
  void updateResolutionOptions();

  QLabel* inviteCodeLabel_;
  QComboBox* displayComboBox_;
  QList<DisplayInfo> displays_;
  QComboBox* resolutionComboBox_;
  QLabel* statsLabel_;
  QPushButton* startButton_;
  QPushButton* stopButton_;
  QPushButton* logButton_;
  QPushButton* backButton_;
  Logger* logger_;
  LogDialog* logDialog_;
  QTimer* frameTimer_;
  int framesInWindow_;
  qint64 bytesInWindow_;
  QElapsedTimer statsTimer_;
  std::unique_ptr<ScreenCapturer> capturer_;
  std::unique_ptr<SessionWriter> writer_;
  QString inviteCode_;
};

}  // namespace screensharing
