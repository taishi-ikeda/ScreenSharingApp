#pragma once

#include <QMainWindow>

class QListWidget;
class QStackedWidget;
class QTimer;

namespace screensharing {

class Presence;
class PresenterPanel;
class ViewerPanel;

// Top-level window: lets the user pick the presenter or viewer role
// and hosts the corresponding panel.
class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);

 private slots:
  void showPresenterPanel();
  void showViewerPanel();
  void showRoleSelection();
  void refreshActiveUsers();

 private:
  QWidget* createRoleSelectionPage();

  QStackedWidget* stackedWidget_;
  QWidget* roleSelectionPage_;
  QListWidget* activeUsersListWidget_;
  Presence* presence_;
  QTimer* activeUsersRefreshTimer_;
  PresenterPanel* presenterPanel_;
  ViewerPanel* viewerPanel_;
};

}  // namespace screensharing
