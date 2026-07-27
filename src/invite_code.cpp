#include "screensharing/invite_code.h"

#include <QRandomGenerator>

#include "screensharing/session_reader.h"

namespace screensharing {

namespace {
constexpr int kMaxAttempts = 10000;
constexpr int kCodeSpace = 10000;
}  // namespace

QString generateAvailableInviteCode() {
  for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
    const int candidate = static_cast<int>(QRandomGenerator::global()->bounded(kCodeSpace));
    const QString code = QString::number(candidate).rightJustified(4, QLatin1Char('0'));
    if (!SessionReader(code).isAlive()) {
      return code;
    }
  }
  return QString();
}

}  // namespace screensharing
