#include "screensharing/log_dialog.h"

#include <QListWidget>
#include <QVBoxLayout>

#include "screensharing/logger.h"

namespace screensharing {

LogDialog::LogDialog(Logger* logger, QWidget* parent)
    : QDialog(parent), logger_(logger), listWidget_(new QListWidget(this)) {
  setWindowTitle(tr("Log"));

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(listWidget_);
  resize(480, 320);

  for (const QString& entry : logger_->entries()) {
    listWidget_->addItem(entry);
  }
  connect(logger_, &Logger::entryAdded, this, &LogDialog::appendEntry);
}

void LogDialog::appendEntry(const QString& formattedEntry) {
  listWidget_->addItem(formattedEntry);
}

}  // namespace screensharing
