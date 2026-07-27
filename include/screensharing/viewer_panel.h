#pragma once

#include <QString>
#include <QWidget>

#include <memory>

class QImage;
class QLabel;
class QLineEdit;
class QPushButton;
class QTimer;

namespace screensharing {

class LogDialog;
class Logger;
class SessionReader;

// UI panel for the "viewer" role.
// Polls SessionReader for frames, decrypts and displays them, preserving
// the presenter's aspect ratio (SPEC.md section 8.1).
class ViewerPanel : public QWidget {
  Q_OBJECT

 public:
  explicit ViewerPanel(QWidget* parent = nullptr);
  ~ViewerPanel() override;

 signals:
  void backRequested();

 private slots:
  void onConnectClicked();
  void onLogClicked();
  void pollForFrame();

 private:
  void updateVideoFrame(const QImage& image);
  void resetConnection();
  void disconnectSession(const QString& reason);

  QLineEdit* codeLineEdit_;
  QPushButton* connectButton_;
  QLabel* statusLabel_;
  QLabel* videoLabel_;
  QPushButton* logButton_;
  QPushButton* backButton_;
  Logger* logger_;
  LogDialog* logDialog_;
  QTimer* pollTimer_;
  std::unique_ptr<SessionReader> reader_;
  QString connectedCode_;
};

}  // namespace screensharing
