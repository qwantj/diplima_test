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
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QPropertyAnimation>
#include <QDir>

#include "common/AppLogger.hpp"
#include "common/DataBridge.hpp"
#include "common/Protocol.hpp"
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
    QTimer* sessionPollTimer_ = nullptr;
};

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("DDoS Dashboard");
    resize(1280, 800);

    setupUI();
    setupSystemTray();

    // Data bridge
    dataBridge_ = new DataBridge(this);
    setupConnections();

    // Connect to collector
    dataBridge_->connectToCollector("localhost", 50050);
    dataBridge_->connectToDatabase("localhost", 5432, "ddos_detection_db", "postgres", "qwerty");

    // Session poll timer
    sessionPollTimer_ = new QTimer(this);
    connect(sessionPollTimer_, &QTimer::timeout, this, &MainWindow::pollSessions);
    sessionPollTimer_->start(10000);
    QTimer::singleShot(1000, this, &MainWindow::pollSessions);
}

void MainWindow::setupUI() {
    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Sidebar
    sidebarList_ = new QListWidget();
    sidebarList_->setFixedWidth(160);
    sidebarList_->setStyleSheet(R"(
        QListWidget {
            background: #1e1e2e; border: none; padding: 8px 0px;
            font-size: 13px; color: #cdd6f4;
        }
        QListWidget::item {
            padding: 10px 12px; margin: 2px 8px; border-radius: 6px;
        }
        QListWidget::item:selected {
            background: #313244; color: #89b4fa;
        }
        QListWidget::item:hover {
            background: #313244;
        }
    )");

    sidebarList_->addItem("■ Dashboard");
    sidebarList_->addItem("■ Deep Analytics");
    sidebarList_->addItem("■ Event Log (Live)");
    sidebarList_->addItem("● Security Incidents");
    sidebarList_->addItem("■ Sessions History");
    sidebarList_->setCurrentRow(0);

    mainLayout->addWidget(sidebarList_);

    // Stacked widget
    stackedWidget_ = new QStackedWidget();

    dashboardWidget_ = new DashboardWidget();
    logWidget_ = new LogWidget();
    sessionWidget_ = new SessionWidget();
    eventHistoryWidget_ = new EventHistoryWidget();

    stackedWidget_->addWidget(dashboardWidget_);
    stackedWidget_->addWidget(dashboardWidget_->getTabAnalytics());
    stackedWidget_->addWidget(logWidget_);
    stackedWidget_->addWidget(eventHistoryWidget_);
    stackedWidget_->addWidget(sessionWidget_);

    mainLayout->addWidget(stackedWidget_, 1);

    connect(sidebarList_, &QListWidget::currentRowChanged,
            stackedWidget_, &QStackedWidget::setCurrentIndex);

    setCentralWidget(centralWidget);

    // Toolbar
    auto* toolbar = addToolBar("Main");
    toolbar->setMovable(false);
    toolbar->setStyleSheet("QToolBar { background: #1e1e2e; border-bottom: 1px solid #313244; padding: 4px; }");

    // Open PCAP button
    auto* openPcapBtn = new QPushButton("📁 Открыть PCAP");
    openPcapBtn->setStyleSheet("padding: 4px 8px; color: #f9e2af; font-weight: bold; background: transparent; border: none;");
    connect(openPcapBtn, &QPushButton::clicked, [this]() {
        QString path = QFileDialog::getOpenFileName(this, "Open PCAP", "", "PCAP files (*.pcap *.pcapng)");
        if (!path.isEmpty()) {
            nlohmann::json data;
            data["path"] = path.toStdString();
            dataBridge_->tcpClient()->sendCommand(Protocol::CMD_LOAD_PCAP, data);
        }
    });
    toolbar->addWidget(openPcapBtn);

    // Folder icon button
    auto* folderBtn = new QPushButton("📁");
    folderBtn->setStyleSheet("padding: 4px 8px; color: #f9e2af; background: transparent; border: none;");
    connect(folderBtn, &QPushButton::clicked, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, "Select PCAP Directory", "", QFileDialog::ShowDirsOnly);
        // Placeholder for directory selection if needed later
    });
    toolbar->addWidget(folderBtn);

    toolbar->addSeparator();

    // Model ComboBox
    toolbar->addWidget(new QLabel("  Модель: "));
    auto* modelCombo = new QComboBox();
    QDir modelsDir("models");
    QStringList modelFiles = modelsDir.entryList(QStringList() << "*.onnx", QDir::Files);
    if (modelFiles.isEmpty()) {
        modelFiles << "mlp_model.onnx" << "rf_model.onnx"; // Fallback
    }
    modelCombo->addItems(modelFiles);
    modelCombo->setStyleSheet("QComboBox { color: #cdd6f4; background: #313244; border: 1px solid #45475a; border-radius: 4px; padding: 2px 8px; }");
    connect(modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, modelCombo](int idx) {
        QString modelName = modelCombo->itemText(idx);
        nlohmann::json data;
        data["path"] = std::string("models/") + modelName.toStdString();
        dataBridge_->tcpClient()->sendCommand(Protocol::CMD_LOAD_MODEL, data);
    });
    toolbar->addWidget(modelCombo);

    toolbar->addSeparator();

    // Live Indicator
    auto* liveIndicator = new QLabel(" ● Live");
    liveIndicator->setStyleSheet("color: #a6e3a1; font-weight: bold; padding: 0px 8px;");
    toolbar->addWidget(liveIndicator);

    // Spacer
    auto* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    // Theme selector
    auto* themeLabel = new QLabel("Theme: ");
    themeLabel->setStyleSheet("color: #cdd6f4;");
    toolbar->addWidget(themeLabel);
    
    auto* themeCombo = new QComboBox();
    themeCombo->addItems({"Dark", "Light", "System"});
    themeCombo->setStyleSheet("QComboBox { color: #cdd6f4; background: #313244; border: 1px solid #45475a; border-radius: 4px; padding: 2px 8px; }");
    connect(themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx) {
        ThemeMode mode = idx == 0 ? ThemeMode::Dark : idx == 1 ? ThemeMode::Light : ThemeMode::System;
        dashboardWidget_->applyTheme(mode);
    });
    toolbar->addWidget(themeCombo);

    // Remove status bar completely
    setStatusBar(nullptr);

    // Apply default theme
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
    trayIcon_->setToolTip("DDoS Monitor");
    trayIcon_->show();

    connect(trayIcon_, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            if (isVisible()) hide(); else show();
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
