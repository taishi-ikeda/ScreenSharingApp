#pragma once

#include <QObject>
#include <QStringList>

class QTimer;

namespace screensharing {

// Announces this running app instance's presence (by OS username) under
// the shared base directory so other instances on the same machine can
// discover who currently has the app open, via a periodically refreshed
// heartbeat file per user.
class Presence : public QObject {
  Q_OBJECT

 public:
  explicit Presence(QObject* parent = nullptr);
  ~Presence() override;

  // Usernames with a fresh-enough heartbeat. Includes this instance's own
  // user; callers typically filter that out themselves.
  static QStringList activeUsers();

 private slots:
  void touchHeartbeat();

 private:
  QTimer* heartbeatTimer_;
};

}  // namespace screensharing
