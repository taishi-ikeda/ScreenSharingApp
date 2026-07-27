#pragma once

#include <QDialog>
#include <QString>

class QListWidget;

namespace screensharing {

class Logger;

// Displays the running log of application/session events.
class LogDialog : public QDialog {
  Q_OBJECT

 public:
  LogDialog(Logger* logger, QWidget* parent = nullptr);

 private slots:
  void appendEntry(const QString& formattedEntry);

 private:
  Logger* logger_;
  QListWidget* listWidget_;
};

}  // namespace screensharing
