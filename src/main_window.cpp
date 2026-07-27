#include "screensharing/main_window.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "screensharing/presence.h"
#include "screensharing/presenter_panel.h"
#include "screensharing/viewer_panel.h"

namespace screensharing {

namespace {
constexpr int kActiveUsersRefreshIntervalMs = 2000;
}  // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      stackedWidget_(new QStackedWidget(this)),
      roleSelectionPage_(nullptr),
      activeUsersListWidget_(nullptr),
      presence_(new Presence(this)),
      activeUsersRefreshTimer_(new QTimer(this)),
      presenterPanel_(new PresenterPanel(this)),
      viewerPanel_(new ViewerPanel(this)) {
  setWindowTitle(tr("ScreenSharing"));

  roleSelectionPage_ = createRoleSelectionPage();

  stackedWidget_->addWidget(roleSelectionPage_);
  stackedWidget_->addWidget(presenterPanel_);
  stackedWidget_->addWidget(viewerPanel_);
  setCentralWidget(stackedWidget_);

  connect(presenterPanel_, &PresenterPanel::backRequested, this,
          &MainWindow::showRoleSelection);
  connect(viewerPanel_, &ViewerPanel::backRequested, this,
          &MainWindow::showRoleSelection);

  activeUsersRefreshTimer_->setInterval(kActiveUsersRefreshIntervalMs);
  connect(activeUsersRefreshTimer_, &QTimer::timeout, this, &MainWindow::refreshActiveUsers);
  activeUsersRefreshTimer_->start();
  refreshActiveUsers();

  resize(480, 400);
}

QWidget* MainWindow::createRoleSelectionPage() {
  auto* page = new QWidget(this);
  auto* currentUserLabel = new QLabel(
      tr("Signed in as: %1").arg(qEnvironmentVariable("USER")), page);
  auto* activeUsersHeaderLabel =
      new QLabel(tr("Other users on this machine:"), page);
  activeUsersListWidget_ = new QListWidget(page);

  auto* shareButton = new QPushButton(tr("Share Screen"), page);
  auto* viewButton = new QPushButton(tr("View Screen"), page);
  auto* aboutQtButton = new QPushButton(tr("About Qt / License"), page);

  auto* buttonRow = new QHBoxLayout;
  buttonRow->addWidget(shareButton);
  buttonRow->addWidget(viewButton);
  buttonRow->addWidget(aboutQtButton);

  auto* layout = new QVBoxLayout(page);
  layout->addWidget(currentUserLabel);
  layout->addWidget(activeUsersHeaderLabel);
  layout->addWidget(activeUsersListWidget_);
  layout->addStretch();
  layout->addLayout(buttonRow);

  connect(shareButton, &QPushButton::clicked, this,
          &MainWindow::showPresenterPanel);
  connect(viewButton, &QPushButton::clicked, this,
          &MainWindow::showViewerPanel);
  connect(aboutQtButton, &QPushButton::clicked, this,
          [this]() { QMessageBox::aboutQt(this); });

  return page;
}

void MainWindow::showPresenterPanel() {
  stackedWidget_->setCurrentWidget(presenterPanel_);
}

void MainWindow::showViewerPanel() {
  stackedWidget_->setCurrentWidget(viewerPanel_);
}

void MainWindow::showRoleSelection() {
  stackedWidget_->setCurrentWidget(roleSelectionPage_);
}

void MainWindow::refreshActiveUsers() {
  const QString currentUser = qEnvironmentVariable("USER");

  QStringList others = Presence::activeUsers();
  others.removeAll(currentUser);
  others.sort();

  activeUsersListWidget_->clear();
  activeUsersListWidget_->addItems(others);
}

}  // namespace screensharing
