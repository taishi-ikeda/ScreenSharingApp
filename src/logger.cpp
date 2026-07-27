#include "screensharing/logger.h"

#include <QDateTime>

namespace screensharing {

Logger::Logger(QObject* parent) : QObject(parent) {}

void Logger::log(const QString& message) {
  const QString formatted =
      QDateTime::currentDateTime().toString(Qt::ISODate) + QStringLiteral(" ") + message;
  entries_.append(formatted);
  emit entryAdded(formatted);
}

QStringList Logger::entries() const { return entries_; }

}  // namespace screensharing
