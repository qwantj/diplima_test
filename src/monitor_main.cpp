#include <QApplication>
#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QToolBar>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QComboBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QPropertyAnimation>
#include <QDir>
#include <QStyle>
#include <QPainter>

#include "common/AppLogger.hpp"
#include "common/DataBridge.hpp"
#include "common/Protocol.hpp"
#include "common/ConfigManager.hpp"
#include "monitor_ui/DashboardWidget.hpp"
#include "monitor_ui/LogWidget.hpp"
#include "monitor_ui/SessionWidget.hpp"
#include "monitor_ui/EventHistoryWidget.hpp"
#include "monitor_ui/ThemePalette.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onCollectorStatusChanged(bool connected);
    void onDatabaseStatusChanged(bool connected);
    void onSessionSelected(int sessionId);
    void pollSessions();

private:
    void setupUI();
    void setupConnections();
    void setupSystemTray();

    DataBridge*          dataBridge_ = nullptr;
    DashboardWidget*     dashboardWidget_ = nullptr;
    LogWidget*           logWidget_ = nullptr;
    SessionWidget*       sessionWidget_ = nullptr;
    EventHistoryWidget*  eventHistoryWidget_ = nullptr;

    QStackedWidget* stackedWidget_ = nullptr;
    QListWidget*    sidebarList_ = nullptr;

    QSystemTrayIcon* trayIcon_ = nullptr;
    QMenu*           trayMenu_ = nullptr;
    QAction*         pauseAction_ = nullptr;
    QTimer*          sessionPollTimer_ = nullptr;

    QIcon            iconNormal_;
    QIcon            iconAttack_;
};

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("DDoS Dashboard");
    resize(1280, 800);

    // Load configuration
    AppConfig config;
    ConfigManager::load("config.json", config);

    // Prepare icons
    iconNormal_ = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
    QPixmap attackPixmap(32, 32);
    attackPixmap.fill(Qt::transparent);
    {
        QPainter p(&attackPixmap);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor("#f38ba8")); // Red
        p.setPen(Qt::NoPen);
        p.drawEllipse(2, 2, 28, 28);
        p.setPen(QPen(Qt::white, 3));
        p.drawLine(10, 10, 22, 22);
        p.drawLine(10, 22, 22, 10);
    }
    iconAttack_ = QIcon(attackPixmap);

    setupUI();
    setupSystemTray();

    // Data bridge
    dataBridge_ = new DataBridge(this);
    setupConnections();

    // Connect using config
    dataBridge_->connectToCollector(QString::fromStdString(config.collectorHost), config.tcpPort);
    dataBridge_->connectToDatabase(QString::fromStdString(config.dbHost), config.dbPort,
                                   QString::fromStdString(config.dbName),
                                   QString::fromStdString(config.dbUser),
                                   QString::fromStdString(config.dbPass));

    // Session poll timer
    sessionPollTimer_ = new QTimer(this);
    connect(sessionPollTimer_, &QTimer::timeout, this, &MainWindow::pollSessions);
    sessionPollTimer_->start(10000);
    QTimer::singleShot(1000, this, &MainWindow::pollSessions);
}

void MainWindow::setupUI() {
    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Green Header Bar
    auto* headerBar = new QWidget();
    headerBar->setFixedHeight(3);
    headerBar->setStyleSheet(QString("background: %1;").arg(ThemePalette::green().name()));
    mainLayout->addWidget(headerBar);

    auto* contentLayout = new QHBoxLayout();
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    mainLayout->addLayout(contentLayout);

    // Sidebar
    sidebarList_ = new QListWidget();
    sidebarList_->setFixedWidth(160);
    sidebarList_->setStyleSheet(QString(R"(
        QListWidget {
            background: %1; border: none; padding: 0px;
            font-size: 13px; color: %2; outline: none;
        }
        QListWidget::item {
            padding: 15px 18px; border-left: 4px solid transparent;
        }
        QListWidget::item:selected {
            background: %3; color: %4; border-left: 4px solid %4;
        }
        QListWidget::item:hover:!selected {
            background: %5;
        }
    )")
    .arg(ThemePalette::mantle().name(), ThemePalette::subtext0().name(),
         ThemePalette::surface0().name(), ThemePalette::green().name(),
         ThemePalette::base().name()));

    sidebarList_->setIconSize(QSize(22, 22));

    auto addSidebarItem = [this](const QString& text, const QColor& color, int iconType) {
        auto* item = new QListWidgetItem(sidebarList_);
        QPixmap pixmap(24, 24);
        pixmap.fill(Qt::transparent);
        QPainter p(&pixmap);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        
        if (iconType == 0) { // Dashboard (Blocks)
            p.drawRoundedRect(2, 2, 8, 8, 2, 2); p.drawRoundedRect(12, 2, 8, 8, 2, 2);
            p.drawRoundedRect(2, 12, 8, 8, 2, 2); p.setBrush(QColor(80, 80, 80)); p.drawRoundedRect(12, 12, 8, 8, 2, 2);
        } else if (iconType == 1) { // Analytics (Chart)
            p.setPen(QPen(color, 2)); p.setBrush(Qt::NoBrush);
            p.drawLine(4, 18, 8, 10); p.drawLine(8, 10, 14, 14); p.drawLine(14, 14, 20, 4);
        } else if (iconType == 2) { // Log
            p.drawRoundedRect(5, 2, 14, 20, 2, 2); p.setBrush(QColor(30, 30, 30));
            p.drawRect(8, 8, 8, 2); p.drawRect(8, 12, 8, 2); p.drawRect(8, 16, 5, 2);
        } else if (iconType == 3) { // Shield
            QPainterPath path; path.moveTo(12, 2); path.lineTo(20, 5); path.lineTo(20, 12);
            path.quadTo(20, 20, 12, 22); path.quadTo(4, 20, 4, 12); path.lineTo(4, 5); path.closeSubpath();
            p.drawPath(path);
        } else { // Sessions
            p.drawRoundedRect(4, 4, 4, 4, 1, 1); p.drawRect(10, 5, 10, 2);
            p.drawRoundedRect(4, 10, 4, 4, 1, 1); p.drawRect(10, 11, 10, 2);
            p.drawRoundedRect(4, 16, 4, 4, 1, 1); p.drawRect(10, 17, 10, 2);
        }
        item->setIcon(QIcon(pixmap));
        item->setText("  " + text);
        sidebarList_->addItem(item);
    };

    addSidebarItem("Dashboard", ThemePalette::green(), 0);
    addSidebarItem("Deep Analytics", ThemePalette::blue(), 1);
    addSidebarItem("Event Log (Live)", ThemePalette::peach(), 2);
    addSidebarItem("Security Incidents", ThemePalette::blue(), 3);
    addSidebarItem("Sessions History", ThemePalette::mauve(), 4);
    
    sidebarList_->setCurrentRow(0);
    contentLayout->addWidget(sidebarList_);

    stackedWidget_ = new QStackedWidget();
    stackedWidget_->setStyleSheet(QString("background: %1;").arg(ThemePalette::crust().name()));
    dashboardWidget_ = new DashboardWidget();
    logWidget_ = new LogWidget();
    sessionWidget_ = new SessionWidget();
    eventHistoryWidget_ = new EventHistoryWidget();

    stackedWidget_->addWidget(dashboardWidget_->getTabAnalytics());  // 0: Dashboard = Infrastructure Health
    stackedWidget_->addWidget(dashboardWidget_);                     // 1: Deep Analytics = Overview (Global State)
    stackedWidget_->addWidget(logWidget_);
    stackedWidget_->addWidget(eventHistoryWidget_);
    stackedWidget_->addWidget(sessionWidget_);

    contentLayout->addWidget(stackedWidget_, 1);
    connect(sidebarList_, &QListWidget::currentRowChanged, stackedWidget_, &QStackedWidget::setCurrentIndex);
    setCentralWidget(centralWidget);

    auto* toolbar = addToolBar("Main");
    toolbar->setMovable(false);
    toolbar->setStyleSheet(QString("QToolBar { background: %1; border-bottom: 1px solid %2; padding: 8px; spacing: 12px; }")
        .arg(ThemePalette::mantle().name(), ThemePalette::surface0().name()));

    auto* openPcapBtn = new QPushButton("Открыть PCAP");
    openPcapBtn->setStyleSheet(QString("QPushButton { padding: 5px 12px; color: %1; font-weight: bold; background: %2; border: 1px solid %3; border-radius: 4px; } QPushButton:hover { background: %3; }")
        .arg(ThemePalette::yellow().name(), ThemePalette::surface0().name(), ThemePalette::surface1().name()));
    connect(openPcapBtn, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Open PCAP", "", "PCAP files (*.pcap *.pcapng)");
        if (!path.isEmpty()) {
            nlohmann::json data; data["path"] = path.toStdString();
            dataBridge_->tcpClient()->sendCommand(Protocol::CMD_LOAD_PCAP, data);
        }
    });
    toolbar->addWidget(openPcapBtn);

    auto* folderBtn = new QPushButton("📂");
    folderBtn->setStyleSheet(QString("QPushButton { padding: 5px 8px; color: %1; background: %2; border: 1px solid %3; border-radius: 4px; } QPushButton:hover { background: %3; }")
        .arg(ThemePalette::yellow().name(), ThemePalette::surface0().name(), ThemePalette::surface1().name()));
    toolbar->addWidget(folderBtn);

    toolbar->addSeparator();
    auto* modelLbl = new QLabel(" Model: "); modelLbl->setStyleSheet(QString("color: %1; font-weight: bold;").arg(ThemePalette::overlay1().name()));
    toolbar->addWidget(modelLbl);
    
    auto* modelCombo = new QComboBox();
    QDir modelsDir("models");
    QStringList modelFiles = modelsDir.entryList(QStringList() << "*.onnx", QDir::Files);
    if (modelFiles.isEmpty()) modelFiles << "rf_model.onnx" << "mlp_model.onnx";
    modelCombo->addItems(modelFiles);
    modelCombo->setStyleSheet(QString("QComboBox { color: %1; background: %2; border: 1px solid %3; border-radius: 4px; padding: 2px 12px; min-width: 150px; }")
        .arg(ThemePalette::text().name(), ThemePalette::surface0().name(), ThemePalette::surface1().name()));
    connect(modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, modelCombo](int idx) {
        QString modelName = modelCombo->itemText(idx);
        nlohmann::json data; data["path"] = std::string("models/") + modelName.toStdString();
        dataBridge_->tcpClient()->sendCommand(Protocol::CMD_LOAD_MODEL, data);
    });
    toolbar->addWidget(modelCombo);

    toolbar->addSeparator();
    auto* liveIndicator = new QWidget(); auto* liveLayout = new QHBoxLayout(liveIndicator);
    liveLayout->setContentsMargins(0, 0, 0, 0); liveLayout->setSpacing(8);
    auto* dot = new QLabel("●"); dot->setStyleSheet(QString("color: %1; font-size: 16px;").arg(ThemePalette::green().name()));
    auto* txt = new QLabel("Live"); txt->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 13px;").arg(ThemePalette::text().name()));
    liveLayout->addWidget(dot); liveLayout->addWidget(txt);
    toolbar->addWidget(liveIndicator);

    auto* spacer = new QWidget(); spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    auto* themeLbl = new QLabel("Theme:");
    themeLbl->setStyleSheet(QString("color: %1; font-weight: bold;").arg(ThemePalette::overlay1().name()));
    toolbar->addWidget(themeLbl);
    auto* themeCombo = new QComboBox();
    themeCombo->addItems({"Dark", "Light"});
    themeCombo->setStyleSheet(QString("QComboBox { color: %1; background: %2; border: 1px solid %3; border-radius: 4px; padding: 2px 12px; }")
        .arg(ThemePalette::text().name(), ThemePalette::surface0().name(), ThemePalette::surface1().name()));
    connect(themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx) {
        ThemeMode mode = idx == 0 ? ThemeMode::Dark : ThemeMode::Light;
        dashboardWidget_->applyTheme(mode);
    });
    toolbar->addWidget(themeCombo);

    setStatusBar(nullptr);
    ThemePalette::apply(ThemeMode::Dark);
}

void MainWindow::setupConnections() {
    // Connection status
    connect(dataBridge_, &DataBridge::collectorConnectionChanged,
            this, &MainWindow::onCollectorStatusChanged);
    connect(dataBridge_, &DataBridge::databaseConnectionChanged,
            this, &MainWindow::onDatabaseStatusChanged);

    // Realtime data
    connect(dataBridge_, &DataBridge::realtimeStatsReceived,
            dashboardWidget_, &DashboardWidget::updateRealtime);
    connect(dataBridge_, &DataBridge::realtimeStatsReceived,
        [this](const DetectionResult& r, uint64_t) {
            logWidget_->addEvent(r);
            
            // Update tray icon based on status
            if (r.label == 1) {
                trayIcon_->setIcon(iconAttack_);
                trayIcon_->setToolTip("DDoS Monitor - ATTACK DETECTED!");
            } else {
                trayIcon_->setIcon(iconNormal_);
                trayIcon_->setToolTip("DDoS Monitor - Normal");
            }
        });
    connect(dataBridge_, &DataBridge::realtimeSnapshotReceived,
            dashboardWidget_, &DashboardWidget::updateSnapshot);

    // Historical data
    connect(dataBridge_, &DataBridge::historicalStatsReceived,
            dashboardWidget_, &DashboardWidget::loadHistory);
    connect(dataBridge_, &DataBridge::historicalStatsReceived,
            logWidget_, &LogWidget::loadHistory);

    // Notifications
    connect(dataBridge_, &DataBridge::notificationReceived,
        [this](const QString& event, const QJsonObject& data) {
            if (event == "replay_done") {
                if (trayIcon_)
                    trayIcon_->showMessage("DDoS Monitor", "Pcap replay completed.",
                        QSystemTrayIcon::Information, 3000);
            }
        });

    // BPF toggle
    connect(dashboardWidget_, &DashboardWidget::bpfToggled,
            dataBridge_, &DataBridge::sendBpfConfig);

    // Session selected
    connect(sessionWidget_, &SessionWidget::sessionSelected,
            this, &MainWindow::onSessionSelected);
}

void MainWindow::setupSystemTray() {
    trayIcon_ = new QSystemTrayIcon(this);
    trayIcon_->setIcon(iconNormal_);
    trayIcon_->setToolTip("DDoS Monitor");

    trayMenu_ = new QMenu(this);
    auto* expandAction = trayMenu_->addAction("Expand Dashboard");
    connect(expandAction, &QAction::triggered, this, &MainWindow::showNormal);

    trayMenu_->addSeparator();
    pauseAction_ = trayMenu_->addAction("Pause Monitoring");
    pauseAction_->setCheckable(true);
    connect(pauseAction_, &QAction::toggled, [this](bool paused) {
        AppLogger::get()->info("Monitoring {}", paused ? "paused" : "resumed");
    });

    auto* exitAction = trayMenu_->addAction("Exit");
    connect(exitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    trayIcon_->setContextMenu(trayMenu_);
    trayIcon_->show();

    connect(trayIcon_, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            if (isVisible()) hide(); else { show(); raise(); activateWindow(); }
        }
    });
}

void MainWindow::onCollectorStatusChanged(bool connected) {
    dashboardWidget_->updateConnectionStatus(connected);
}

void MainWindow::onDatabaseStatusChanged(bool connected) {
    if (connected) {
        eventHistoryWidget_->setDatabaseManager(dataBridge_->databaseManager());
    }
}

void MainWindow::onSessionSelected(int sessionId) {
    if (!dataBridge_->databaseManager()) return;
    auto events = dataBridge_->databaseManager()->getEventsForSession(sessionId);
    dashboardWidget_->loadHistory(events);
    logWidget_->loadHistory(events);
    sidebarList_->setCurrentRow(0); // switch to dashboard
}

void MainWindow::pollSessions() {
    if (dataBridge_->databaseManager() && dataBridge_->databaseManager()->isConnected()) {
        auto sessions = dataBridge_->databaseManager()->getSessions();
        sessionWidget_->loadSessions(sessions);
    }
}

// Must include MOC for MainWindow defined in .cpp
#include "monitor_main.moc"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("ddos_monitor");
    app.setApplicationVersion("2.2");
    app.setOrganizationName("DiplomDDoS");

    AppLogger::init("ddos_monitor.log");

    // Register meta types
    qRegisterMetaType<DetectionResult>();
    qRegisterMetaType<std::vector<DetectionResult>>();
    qRegisterMetaType<SessionInfo>();
    qRegisterMetaType<std::vector<SessionInfo>>();
    qRegisterMetaType<std::vector<QByteArray>>();
    qRegisterMetaType<ThemeMode>();

    MainWindow w;
    w.show();

    return app.exec();
}
