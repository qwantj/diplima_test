/**
 * @file collector_main.cpp
 * @brief Точка входа подсистемы ddos_collector.
 *
 * Назначение: Инициализация компонентов системы, разбор аргументов командной строки и запуск циклов мониторинга.
 * Входные данные: Аргументы командной строки (интерфейс, модель, параметры БД).
 * Результаты: Запуск сервиса сбора и анализа трафика.
 * Метод решения: Использование QCoreApplication для управления жизненным циклом программы и обработки сигналов.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTimer>
#include <QDateTime>

#include "common/AppLogger.hpp"
#include "common/Protocol.hpp"
#include "common/TcpServer.hpp"
#include "common/DatabaseManager.hpp"
#include "common/ConfigManager.hpp"
#include "common/SystemMetricsCollector.hpp"
#include "core/DetectionEngine.hpp"

#include <csignal>
#include <atomic>
#include <QNetworkInterface>
#include <QHostAddress>

#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#endif

// Глобальный флаг работы приложения
static std::atomic<bool> g_running{true};

/**
 * @brief Обработчик сигналов завершения (SIGINT, SIGTERM).
 */
static void signalHandler(int) {
  g_running = false;
  QCoreApplication::quit();
}

/**
 * @brief Преобразование псевдонима интерфейса (например, "WiFi") в IP или системное имя.
 */
static std::string resolveNetworkInterface(const std::string& userInput) {
  QString input = QString::fromStdString(userInput).trimmed().remove(' ').remove('-').remove('_');
  bool isWiFiQuery = (input.compare("WiFi", Qt::CaseInsensitive) == 0);

  for (const QNetworkInterface& netIface : QNetworkInterface::allInterfaces()) {
    QString friendlyName = netIface.humanReadableName();
    QString systemName = netIface.name();
    QString cleanFriendly = friendlyName.simplified().remove(' ').remove('-').remove('_');
    QString cleanSystem = systemName.simplified().remove(' ').remove('-').remove('_');

    bool match = (cleanFriendly.compare(input, Qt::CaseInsensitive) == 0) ||
           (cleanSystem.compare(input, Qt::CaseInsensitive) == 0);
    
    if (!match && isWiFiQuery) {
      if (friendlyName.contains("Беспроводная", Qt::CaseInsensitive) || 
        friendlyName.contains("сеть", Qt::CaseInsensitive)) {
        match = true;
      }
    }

    if (match) {
      for (const QNetworkAddressEntry& entry : netIface.addressEntries()) {
        if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
          std::string ipStr = entry.ip().toString().toStdString();
          AppLogger::get()->info("Resolved alias '{}' to IP: {}", userInput, ipStr);
          return ipStr;
        }
      }
    }
  }
  return userInput;
}

/**
 * @brief Главная функция программы.
 */
int main(int argc, char* argv[]) {
#ifdef Q_OS_WIN
  // Настройка кодировки консоли для корректного вывода кириллицы
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif

  QCoreApplication app(argc, argv);
  app.setApplicationName("ddos_collector");
  app.setApplicationVersion("2.2");

  // Инициализация логгера
  AppLogger::init("ddos_collector.log");

  // Загрузка конфигурации из JSON
  AppConfig config;
  ConfigManager::load("config.json", config);

  // Настройка парсера аргументов командной строки
  QCommandLineParser parser;
  parser.setApplicationDescription("DDoS Detection Collector - Подсистема сбора и анализа");
  parser.addHelpOption();
  parser.addVersionOption();

  parser.addOptions({
    {{"i", "interface"}, "Имя сетевого интерфейса", "name", QString::fromStdString(config.defaultInterface)},
    {{"f", "pcap"},     "Путь к pcap-файлу для анализа", "file"},
    {{"m", "model"},    "Путь к ONNX модели", "path", QString::fromStdString(config.defaultModel)},
    {{"e", "ep"},       "Провайдер исполнения (cpu|cuda|dml)", "ep", QString::fromStdString(config.defaultEp)},
    {"list-interfaces", "Вывести список доступных интерфейсов"},
    {"tcp-host",        "Адрес TCP-сервера", "host", QString::fromStdString(config.tcpBindHost)},
    {"tcp-port",        "Порт TCP-сервера", "port", QString::number(config.tcpPort)},
    {"db-host",         "Хост PostgreSQL", "host", QString::fromStdString(config.dbHost)},
    {"db-port",         "Порт PostgreSQL", "port", QString::number(config.dbPort)},
    {"db-name",         "Имя базы данных", "name", QString::fromStdString(config.dbName)},
    {"db-user",         "Пользователь БД", "user", QString::fromStdString(config.dbUser)},
    {"db-password",     "Пароль БД", "pass", QString::fromStdString(config.dbPass)},
    {"pcap-dir",        "Директория для дампов", "dir"},
  });

  parser.process(app);

  // Регистрация типов для сигнально-слотовой связи Qt
  qRegisterMetaType<DetectionResult>();
  qRegisterMetaType<std::vector<QByteArray>>();

  // Вывод списка интерфейсов при необходимости
  if (parser.isSet("list-interfaces")) {
    AppLogger::get()->info("Доступные сетевые интерфейсы:");
    for (const QNetworkInterface& netIface : QNetworkInterface::allInterfaces()) {
      if (netIface.flags().testFlag(QNetworkInterface::IsUp)) {
        QString ipList;
        for (const QNetworkAddressEntry& entry : netIface.addressEntries()) {
          if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
            if (!ipList.isEmpty()) ipList += ", ";
            ipList += entry.ip().toString();
          }
        }
        if (!ipList.isEmpty()) {
          AppLogger::get()->info("  \"{}\" (IP: {})", 
            netIface.humanReadableName().toUtf8().constData(), 
            ipList.toUtf8().constData());
        }
      }
    }
    return 0;
  }

  // Проверка обязательных параметров
  if (!parser.isSet("interface") && !parser.isSet("pcap")) {
    AppLogger::get()->error("Необходимо указать либо --interface, либо --pcap.");
    parser.showHelp(1);
  }

  // Установка обработчиков сигналов ОС
  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);

  // Инициализация ядра обнаружения
  DetectionEngine engine;
  std::string modelPath = parser.value("model").toStdString();
  std::string ep = parser.value("ep").toStdString();

  if (!engine.init(modelPath, "", ep)) {
    AppLogger::get()->error("Ошибка инициализации ядра обнаружения.");
    return 1;
  }

  // Запуск TCP-сервера для связи с ddos_monitor
  TcpServer tcpServer;
  QString tcpHost = parser.value("tcp-host");
  quint16 tcpPort = parser.value("tcp-port").toUShort();
  if (!tcpServer.startListening(tcpHost, tcpPort)) {
    AppLogger::get()->error("Ошибка запуска TCP сервера на {} порт {}", tcpHost.toStdString(), tcpPort);
    return 1;
  }

  // Инициализация менеджера базы данных
  DatabaseManager dbManager;
  bool dbConnected = dbManager.connectToDatabase(
    parser.value("db-host"),
    parser.value("db-port").toInt(),
    parser.value("db-name"),
    parser.value("db-user"),
    parser.value("db-password"));

  // Счетчики сессии
  uint64_t totalPackets = 0;
  uint64_t attackCount = 0;
  uint64_t benignCount = 0;
  int sessionId = -1;

  // Лямбда для управления сессиями БД
  auto createNewSession = [&](const QString& iface, const QString& model) {
    if (dbConnected) {
      if (sessionId > 0) {
        dbManager.closeSession(sessionId, totalPackets, attackCount, benignCount);
        totalPackets = 0; attackCount = 0; benignCount = 0;
      }
      sessionId = dbManager.createSession(iface, model);
    }
  };

  if (dbConnected) {
    createNewSession(
      parser.isSet("interface") ? parser.value("interface") : "pcap_replay",
      QString::fromStdString(engine.modelInferencer().modelName()));
  }

  // Состояние режима воспроизведения PCAP (по запросу из GUI)
  DetectionEngine* replayEngine = nullptr;
  std::atomic<bool> isReplaying{false};
  int replaySessionId = -1;
  std::atomic<uint64_t> replayTotalPackets{0};

  // Состояние агрегации инцидентов для БД
  bool inAttack = false;
  QDateTime attackStartTime;
  float maxPpsSeen = 0;
  float totalConf = 0;
  int samples = 0;
  std::string attacker = "unknown";

  // Вспомогательная функция обработки результатов для записи в БД
  auto processResultForDb = [&](const DetectionResult& r, int sid) {
    if (!dbConnected || sid <= 0) return;
    
    dbManager.enqueueEvent(r);
    dbManager.enqueueSnapshot((float)r.pps, totalPackets, r.label, sid);

    // Логика объединения посекундных срабатываний в один инцидент безопасности
    if (r.label == 1 && r.confidence > 0.4) {
      if (!inAttack) {
        inAttack = true;
        attackStartTime = r.timestamp.isValid() ? r.timestamp : QDateTime::currentDateTime();
        maxPpsSeen = (float)r.pps;
        totalConf = r.confidence;
        samples = 1;
        attacker = r.topTalkers.empty() ? "unknown" : r.topTalkers[0].first;
      } else {
        maxPpsSeen = (std::max)(maxPpsSeen, (float)r.pps);
        totalConf += r.confidence;
        samples++;
      }
    } else if (inAttack) {
      // Атака завершилась — сохраняем итог
      float duration = attackStartTime.secsTo(r.timestamp.isValid() ? r.timestamp : QDateTime::currentDateTime());
      if (duration <= 0) duration = (float)samples; 
      
      dbManager.enqueueSecurityEvent(sid, attackStartTime, duration,
        QString::fromStdString(attacker), maxPpsSeen,
        QString::fromStdString(r.modelName), totalConf / samples);
      inAttack = false;
    }
  };

  // Обработка команд управления от монитора
  QObject::connect(&tcpServer, &TcpServer::commandReceived, [&](const std::string& cmd, const nlohmann::json& data) {
    if (cmd == Protocol::CMD_LOAD_PCAP) {
      std::string action = data.value("action", "");
      if (action == "stop_replay") {
        if (isReplaying) {
          isReplaying = false;
          if (replayEngine) { replayEngine->stop(); delete replayEngine; replayEngine = nullptr; }
          if (dbConnected && replaySessionId > 0) {
            dbManager.closeSession(replaySessionId, 0, 0, 0);
            replaySessionId = -1;
          }
          auto notifyMsg = Protocol::serializeNotify("live_resumed", {{"session_id", sessionId}});
          tcpServer.broadcast(notifyMsg);
        }
        return;
      }

      std::string path = data.value("path", "");
      if (!path.empty()) {
        AppLogger::get()->info("Начато воспроизведение файла: {}", path);
        if (replayEngine) { replayEngine->stop(); delete replayEngine; }
        
        auto startNotify = Protocol::serializeNotify("replay_started", {{"path", path}});
        tcpServer.broadcast(startNotify);

        replayEngine = new DetectionEngine();
        replayEngine->init(modelPath, "", ep);
        replayTotalPackets = 0;

        if (dbConnected) {
          replaySessionId = dbManager.createSession(QString::fromStdString("pcap:" + path), 
                                QString::fromStdString(replayEngine->modelInferencer().modelName()));
        }

        replayEngine->setResultCallback([&](const DetectionResult& result) {
          if (!isReplaying) return;
          DetectionResult r = result;
          r.sessionId = replaySessionId;
          replayTotalPackets += r.totalPackets;
          
          QByteArray statsMsg = Protocol::serializeStats(r, replayTotalPackets.load());
          QMetaObject::invokeMethod(&tcpServer, [&tcpServer, statsMsg]() {
            tcpServer.broadcast(statsMsg);
          }, Qt::QueuedConnection);

          processResultForDb(r, replaySessionId);
        });

        if (replayEngine->startReplay(path)) {
          isReplaying = true;
        }
      }
    } else if (cmd == Protocol::CMD_LOAD_MODEL) {
      std::string path = data.value("path", "");
      if (!path.empty()) {
        engine.hotSwapModel(path);
        if (replayEngine) replayEngine->hotSwapModel(path);
      }
    } else if (cmd == Protocol::CMD_CONFIG_BPF) {
      bool enable = data.value("enable", false);
      engine.setMitigationEnabled(enable);
      if (replayEngine) replayEngine->setMitigationEnabled(enable);
      AppLogger::get()->info("Активная защита: {}", enable ? "ВКЛЮЧЕНА" : "ВЫКЛЮЧЕНА");
    } else if (cmd == Protocol::CMD_CONFIG_DUMP) {
      bool enable = data.value("enable", false);
      if (enable) engine.enablePcapDump("pcap_dumps");
      else engine.disablePcapDump();
    } else if (cmd == Protocol::CMD_STOP) {
      g_running = false;
      QCoreApplication::quit();
    }
  });

  // Основной коллбэк результатов работы живого мониторинга
  engine.setResultCallback([&](const DetectionResult& result) {
    DetectionResult r = result;
    r.sessionId = sessionId;
    totalPackets += r.totalPackets;
    if (r.label == 1) attackCount++; else benignCount++;

    if (!isReplaying) {
      // Отправка live-статистики подключенным клиентам
      QByteArray statsMsg = Protocol::serializeStats(r, totalPackets);
      QMetaObject::invokeMethod(&tcpServer, [&tcpServer, statsMsg]() {
        tcpServer.broadcast(statsMsg);
      }, Qt::QueuedConnection);

      QByteArray snapMsg = Protocol::serializeSnapshot((float)r.pps, totalPackets, r.label);
      QMetaObject::invokeMethod(&tcpServer, [&tcpServer, snapMsg]() {
        tcpServer.broadcast(snapMsg);
      }, Qt::QueuedConnection);
    }

    processResultForDb(r, sessionId);
  });

  // Старт работы
  if (parser.isSet("pcap")) {
    engine.startReplay(parser.value("pcap").toStdString());
  } else {
    std::string iface = resolveNetworkInterface(parser.value("interface").toStdString());
    engine.startLive(iface);
  }

  // Запуск цикла обработки событий Qt
  int ret = app.exec();

  // Корректное завершение
  engine.stop();
  if (replayEngine) { replayEngine->stop(); delete replayEngine; }
  if (dbConnected && sessionId > 0) {
    dbManager.closeSession(sessionId, totalPackets, attackCount, benignCount);
  }
  
  return ret;
}
