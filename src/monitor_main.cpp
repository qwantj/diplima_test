/**
 * @file monitor_main.cpp
 * @brief Точка входа подсистемы ddos_monitor.
 *
 * Назначение: Инициализация графического интерфейса пользователя и настройка связей с коллектором.
 * Входные данные: Параметры конфигурации из config.json.
 * Результаты: Отображение рабочего стола оператора безопасности.
 * Метод решения: Использование QApplication и QMainWindow для построения интерактивного интерфейса на Qt6.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

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

// Перечисление иконок боковой панели
enum class SidebarIcon {
  Dashboard,
  DeepAnalytics,
  EventLog,
  SecurityIncidents,
  SessionsHistory
};

/**
 * @class MainWindow
 * @brief Главное окно приложения мониторинга.
 */
class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(QWidget* parent = nullptr);

private slots:
  // Обработчики изменения статуса соединений
  void onCollectorStatusChanged(bool connected);
  void onDatabaseStatusChanged(bool connected);
  void onSessionSelected(int sessionId);
  void pollSessions();

private:
  void setupUI();
  void setupConnections();
  void setupSystemTray();

  // Основные виджеты интерфейса
  DataBridge*          dataBridge_ = nullptr;
  DashboardWidget*     dashboardWidget_ = nullptr;
  LogWidget*           logWidget_ = nullptr;
  SessionWidget*       sessionWidget_ = nullptr;
  EventHistoryWidget*  eventHistoryWidget_ = nullptr;

  QLabel* dbDot_ = nullptr;
  QLabel* dbTxt_ = nullptr;

  QStackedWidget* stackedWidget_ = nullptr;
  QListWidget*    sidebarList_ = nullptr;

  QSystemTrayIcon* trayIcon_ = nullptr;
  QMenu*           trayMenu_ = nullptr;
  QAction*         pauseAction_ = nullptr;
  QTimer*          sessionPollTimer_ = nullptr;

  QIcon            iconNormal_;
  QIcon            iconAttack_;
};

#include <QToolTip>
#include <QMouseEvent>

/**
 * @class RightClickToolTipFilter
 * @brief Фильтр событий для отображения подсказок по правой кнопке мыши.
 */
class RightClickToolTipFilter : public QObject {
public:
  explicit RightClickToolTipFilter(QObject* parent = nullptr) : QObject(parent) {}
protected:
  bool eventFilter(QObject* obj, QEvent* event) override {
    if (event->type() == QEvent::MouseButtonPress) {
      auto* mouseEvent = static_cast<QMouseEvent*>(event);
      if (mouseEvent->button() == Qt::RightButton) {
        auto* widget = qobject_cast<QWidget*>(obj);
        if (widget && !widget->toolTip().isEmpty()) {
          QToolTip::showText(mouseEvent->globalPosition().toPoint(), widget->toolTip(), widget);
          return true;
        }
      }
    }
    return QObject::eventFilter(obj, event);
  }
};

// Конструктор главного окна
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  qApp->installEventFilter(new RightClickToolTipFilter(this));

  setWindowTitle("DDoS Dashboard - Система мониторинга");
  resize(1280, 800);

  // Загрузка настроек
  AppConfig config;
  ConfigManager::load("config.json", config);

  // Подготовка иконок состояния (норма/атака)
  iconNormal_ = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
  QPixmap attackPixmap(32, 32);
  attackPixmap.fill(Qt::transparent);
  {
    QPainter p(&attackPixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor("#f38ba8")); 
    p.setPen(Qt::NoPen);
    p.drawEllipse(2, 2, 28, 28);
    p.setPen(QPen(Qt::white, 3));
    p.drawLine(10, 10, 22, 22);
    p.drawLine(10, 22, 22, 10);
  }
  iconAttack_ = QIcon(attackPixmap);

  setupUI();           // Построение интерфейса
  setupSystemTray();   // Настройка трея

  // Инициализация моста данных
  dataBridge_ = new DataBridge(this);
  setupConnections();

  // Автоматическое подключение согласно конфигу
  dataBridge_->connectToCollector(QString::fromStdString(config.collectorHost), config.tcpPort);
  dataBridge_->connectToDatabase(QString::fromStdString(config.dbHost), config.dbPort,
                   QString::fromStdString(config.dbName),
                   QString::fromStdString(config.dbUser),
                   QString::fromStdString(config.dbPass));

  // Таймер опроса списка сессий
  sessionPollTimer_ = new QTimer(this);
  connect(sessionPollTimer_, &QTimer::timeout, this, &MainWindow::pollSessions);
  sessionPollTimer_->start(10000);
  QTimer::singleShot(1000, this, &MainWindow::pollSessions);
}

/**
 * @brief Настройка элементов управления и макета окна.
 */
void MainWindow::setupUI() {
  auto* centralWidget = new QWidget(this);
  auto* mainLayout = new QVBoxLayout(centralWidget);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // Верхняя статусная полоса (зеленая)
  auto* headerBar = new QWidget();
  headerBar->setFixedHeight(3);
  headerBar->setStyleSheet(QString("background: %1;").arg(ThemePalette::green().name()));
  mainLayout->addWidget(headerBar);

  auto* contentLayout = new QHBoxLayout();
  contentLayout->setContentsMargins(0, 0, 0, 0);
  contentLayout->setSpacing(0);
  mainLayout->addLayout(contentLayout);

  // Боковая панель навигации (Sidebar)
  sidebarList_ = new QListWidget();
  sidebarList_->setFixedWidth(185);
  sidebarList_->setProperty("cssClass", "sidebarList");
  sidebarList_->setIconSize(QSize(22, 22));

  // Лямбда для быстрого добавления пунктов меню
  auto addSidebarItem = [this](const QString& text, const QColor& color, SidebarIcon iconType, const QString& tooltip) {
    auto* item = new QListWidgetItem(sidebarList_);
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(color);
    p.setPen(Qt::NoPen);

    // Отрисовка стилизованных иконок через QPainter
    switch (iconType) {
      case SidebarIcon::Dashboard:
        p.drawRoundedRect(2, 2, 8, 8, 2, 2); p.drawRoundedRect(12, 2, 8, 8, 2, 2);
        p.drawRoundedRect(2, 12, 8, 8, 2, 2); p.setBrush(QColor(80, 80, 80)); p.drawRoundedRect(12, 12, 8, 8, 2, 2);
        break;
      case SidebarIcon::DeepAnalytics:
        p.setPen(QPen(color, 2)); p.setBrush(Qt::NoBrush);
        p.drawLine(4, 18, 8, 10); p.drawLine(8, 10, 14, 14); p.drawLine(14, 14, 20, 4);
        break;
      case SidebarIcon::EventLog:
        p.drawRoundedRect(5, 2, 14, 20, 2, 2); p.setBrush(QColor(30, 30, 30));
        p.drawRect(8, 8, 8, 2); p.drawRect(8, 12, 8, 2); p.drawRect(8, 16, 5, 2);
        break;
      case SidebarIcon::SecurityIncidents: {
        QPainterPath path; path.moveTo(12, 2); path.lineTo(20, 5); path.lineTo(20, 12);
        path.quadTo(20, 20, 12, 22); path.quadTo(4, 20, 4, 12); path.lineTo(4, 5); path.closeSubpath();
        p.drawPath(path);
        break;
      }
      case SidebarIcon::SessionsHistory:
        p.drawRoundedRect(4, 4, 4, 4, 1, 1); p.drawRect(10, 5, 10, 2);
        p.drawRoundedRect(4, 10, 4, 4, 1, 1); p.drawRect(10, 11, 10, 2);
        p.drawRoundedRect(4, 16, 4, 4, 1, 1); p.drawRect(10, 17, 10, 2);
        break;
    }
    item->setIcon(QIcon(pixmap));
    item->setText("  " + text);
    item->setToolTip(tooltip);
    sidebarList_->addItem(item);
  };

  // Наполнение меню
  addSidebarItem("Обзор", ThemePalette::green(), SidebarIcon::Dashboard, "Главная панель: Общее состояние и основные графики");
  addSidebarItem("Аналитика", ThemePalette::blue(), SidebarIcon::DeepAnalytics, "Глубокая аналитика: Топология сети и распределение трафика");
  addSidebarItem("Лог событий", ThemePalette::peach(), SidebarIcon::EventLog, "Подробный список всех классифицированных потоков");
  addSidebarItem("Инциденты", ThemePalette::blue(), SidebarIcon::SecurityIncidents, "История обнаруженных атак");
  addSidebarItem("История сессий", ThemePalette::mauve(), SidebarIcon::SessionsHistory, "Список всех прошлых запусков системы");
  
  sidebarList_->setCurrentRow(0);
  contentLayout->addWidget(sidebarList_);

  // Стек виджетов для переключения страниц
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

  contentLayout->addWidget(stackedWidget_, 1);
  connect(sidebarList_, &QListWidget::currentRowChanged, stackedWidget_, &QStackedWidget::setCurrentIndex);
  setCentralWidget(centralWidget);

  // Панель инструментов (Toolbar)
  auto* toolbar = addToolBar("Main");
  toolbar->setMovable(false);

  auto* openPcapBtn = new QPushButton("Открыть PCAP");
  openPcapBtn->setProperty("cssClass", "toolbarBtnYellow");
  connect(openPcapBtn, &QPushButton::clicked, [this]() {
    QString path = QFileDialog::getOpenFileName(this, "Открыть PCAP", "", "PCAP файлы (*.pcap *.pcapng)");
    if (!path.isEmpty()) {
      nlohmann::json data; data["path"] = path.toStdString();
      dataBridge_->tcpClient()->sendCommand(Protocol::CMD_LOAD_PCAP, data);
    }
  });
  toolbar->addWidget(openPcapBtn);

  auto* liveReturnBtn = new QPushButton("⏹ Live");
  liveReturnBtn->setToolTip("Вернуться к живому трафику");
  liveReturnBtn->setProperty("cssClass", "toolbarBtnGreen");
  connect(liveReturnBtn, &QPushButton::clicked, [this]() {
    nlohmann::json data; data["action"] = "stop_replay";
    dataBridge_->tcpClient()->sendCommand(Protocol::CMD_LOAD_PCAP, data);
  });
  toolbar->addWidget(liveReturnBtn);

  auto* recordBtn = new QPushButton("⏺ Запись");
  recordBtn->setCheckable(true);
  recordBtn->setToolTip("Сохранение дампов трафика на диск");
  recordBtn->setProperty("cssClass", "toolbarBtnPeach");
  
  connect(recordBtn, &QPushButton::toggled, [this, recordBtn](bool checked) {
    if (checked) {
      recordBtn->setText("⏹ Стоп");
      recordBtn->setProperty("cssClass", "toolbarBtnRed");
    } else {
      recordBtn->setText("⏺ Запись");
      recordBtn->setProperty("cssClass", "toolbarBtnPeach");
    }
    recordBtn->style()->unpolish(recordBtn); recordBtn->style()->polish(recordBtn);
    dataBridge_->sendDumpConfig(checked);
  });
  toolbar->addWidget(recordBtn);

  toolbar->addSeparator();
  toolbar->addWidget(new QLabel(" Модель: "));
  
  auto* modelCombo = new QComboBox();
  QDir modelsDir("models");
  QStringList modelFiles = modelsDir.entryList(QStringList() << "*.onnx", QDir::Files);
  if (modelFiles.isEmpty()) modelFiles << "xgb_model.onnx" << "rf_model.onnx" << "mlp_model.onnx";
  modelCombo->addItems(modelFiles);
  connect(modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, modelCombo](int idx) {
    QString modelName = modelCombo->itemText(idx);
    nlohmann::json data; data["path"] = std::string("models/") + modelName.toStdString();
    dataBridge_->tcpClient()->sendCommand(Protocol::CMD_LOAD_MODEL, data);
  });
  toolbar->addWidget(modelCombo);

  // Индикаторы статуса
  toolbar->addSeparator();
  auto* dbIndicator = new QWidget(); auto* dbLayout = new QHBoxLayout(dbIndicator);
  dbLayout->setContentsMargins(0, 0, 0, 0); dbLayout->setSpacing(8);
  dbDot_ = new QLabel("●"); dbDot_->setProperty("cssClass", "statusLabelRed");
  dbTxt_ = new QLabel("БД: Выкл"); dbTxt_->setProperty("cssClass", "statusLabelText");
  dbLayout->addWidget(dbDot_); dbLayout->addWidget(dbTxt_);
  toolbar->addWidget(dbIndicator);

  auto* spacer = new QWidget(); spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  toolbar->addWidget(spacer);

  auto* themeCombo = new QComboBox();
  themeCombo->addItems({"Темная", "Светлая"});
  connect(themeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx) {
    dashboardWidget_->applyTheme(idx == 0 ? ThemeMode::Dark : ThemeMode::Light);
  });
  toolbar->addWidget(themeCombo);

  ThemePalette::apply(ThemeMode::Dark);
}

/**
 * @brief Установка соединений между мостом данных и виджетами.
 */
void MainWindow::setupConnections() {
  connect(dataBridge_, &DataBridge::collectorConnectionChanged, this, &MainWindow::onCollectorStatusChanged);
  connect(dataBridge_, &DataBridge::databaseConnectionChanged, this, &MainWindow::onDatabaseStatusChanged);

  // Передача оперативных данных на графики
  connect(dataBridge_, &DataBridge::realtimeStatsReceived, dashboardWidget_, &DashboardWidget::updateRealtime);
  connect(dataBridge_, &DataBridge::realtimeStatsReceived, [this](const DetectionResult& r, uint64_t) {
    logWidget_->addEvent(r);
    // Обновление иконки трея в зависимости от наличия атаки
    trayIcon_->setIcon(r.label == 1 ? iconAttack_ : iconNormal_);
  });

  // Загрузка истории сессии
  connect(sessionWidget_, &SessionWidget::sessionSelected, this, &MainWindow::onSessionSelected);
}

// Настройка системного трея
void MainWindow::setupSystemTray() {
  trayIcon_ = new QSystemTrayIcon(this);
  trayIcon_->setIcon(iconNormal_);
  trayIcon_->setToolTip("DDoS Monitor");

  trayMenu_ = new QMenu(this);
  trayMenu_->addAction("Развернуть", this, &MainWindow::showNormal);
  trayMenu_->addSeparator();
  trayMenu_->addAction("Выход", qApp, &QCoreApplication::quit);

  trayIcon_->setContextMenu(trayMenu_);
  trayIcon_->show();
}

void MainWindow::onCollectorStatusChanged(bool connected) {
  dashboardWidget_->updateConnectionStatus(connected);
}

void MainWindow::onDatabaseStatusChanged(bool connected) {
  if (dbDot_ && dbTxt_) {
    dbDot_->setProperty("cssClass", connected ? "statusLabelGreen" : "statusLabelRed");
    dbTxt_->setText(connected ? "БД: Подключено" : "БД: Ошибка");
    dbDot_->style()->unpolish(dbDot_); dbDot_->style()->polish(dbDot_);
    dbTxt_->style()->unpolish(dbTxt_); dbTxt_->style()->polish(dbTxt_);
  }
  if (connected) {
    eventHistoryWidget_->setDatabaseManager(dataBridge_->databaseManager());
    pollSessions();
  }
}

void MainWindow::onSessionSelected(int sessionId) {
  if (!dataBridge_->databaseManager()) return;
  auto events = dataBridge_->databaseManager()->getEventsForSession(sessionId);
  dashboardWidget_->loadHistory(events);
  logWidget_->loadHistory(events);
  sidebarList_->setCurrentRow(0);
}

void MainWindow::pollSessions() {
  if (dataBridge_->databaseManager() && dataBridge_->databaseManager()->isConnected()) {
    auto sessions = dataBridge_->databaseManager()->getSessions();
    sessionWidget_->loadSessions(sessions);
  }
}

#include "monitor_main.moc"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName("ddos_monitor");
  app.setApplicationVersion("2.2");

  AppLogger::init("ddos_monitor.log");

  // Регистрация типов данных для сигналов
  qRegisterMetaType<DetectionResult>();
  qRegisterMetaType<std::vector<DetectionResult>>();
  qRegisterMetaType<SessionInfo>();
  qRegisterMetaType<ThemeMode>();

  MainWindow w;
  w.show();

  return app.exec();
}
