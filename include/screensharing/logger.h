#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

namespace screensharing {

// In-memory log store shown to the user via LogDialog.
class Logger : public QObject {
  Q_OBJECT

 public:
  explicit Logger(QObject* parent = nullptr);

  void log(const QString& message);
  QStringList entries() const;

 signals:
  void entryAdded(const QString& formattedEntry);

 private:
  QStringList entries_;
};

}  // namespace screensharing
