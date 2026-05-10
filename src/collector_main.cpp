#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTimer>

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
#include <windows.h>
#endif

static std::atomic<bool> g_running{true};

static void signalHandler(int) {
    g_running = false;
    QCoreApplication::quit();
}

// Поиск IP-адреса по человекочитаемому имени (например, "Wi-Fi" или "Ethernet")
static std::string resolveNetworkInterface(const std::string& userInput) {
    QString input = QString::fromStdString(userInput).trimmed().remove(' ').remove('-').remove('_');
    
    // Специальный маппинг для Wi-Fi в русской локализации
    bool isWiFiQuery = (input.compare("WiFi", Qt::CaseInsensitive) == 0);

    for (const QNetworkInterface& netIface : QNetworkInterface::allInterfaces()) {
        QString friendlyName = netIface.humanReadableName();
        QString systemName = netIface.name();
        
        QString cleanFriendly = friendlyName.simplified().remove(' ').remove('-').remove('_');
        QString cleanSystem = systemName.simplified().remove(' ').remove('-').remove('_');

        bool match = (cleanFriendly.compare(input, Qt::CaseInsensitive) == 0) ||
                     (cleanSystem.compare(input, Qt::CaseInsensitive) == 0);
        
        // Если ищем wifi, а нашли "Беспроводная сеть" (или ее вариант в кракозябрах)
        if (!match && isWiFiQuery) {
            if (friendlyName.contains("Беспроводная", Qt::CaseInsensitive) || 
                friendlyName.contains("СЃРµС‚СЊ", Qt::CaseInsensitive)) {
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

#ifdef Q_OS_WIN
#include <windows.h>
#endif

int main(int argc, char* argv[]) {
#ifdef Q_OS_WIN
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    QCoreApplication app(argc, argv);
    app.setApplicationName("ddos_collector");
    app.setApplicationVersion("2.2");

    AppLogger::init("ddos_collector.log");

    // Load configuration
    AppConfig config;
    ConfigManager::load("config.json", config);

    QCommandLineParser parser;
    parser.setApplicationDescription("DDoS Detection Collector");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOptions({
        {{"i", "interface"}, "Network interface name", "name", QString::fromStdString(config.defaultInterface)},
        {{"f", "pcap"},     "Pcap file to replay", "file"},
        {{"m", "model"},    "ONNX model path", "path", QString::fromStdString(config.defaultModel)},
        {{"e", "ep"},       "Execution provider (cpu|cuda|dml)", "ep", QString::fromStdString(config.defaultEp)},
        {"list-interfaces", "List available network interfaces"},
        {"tcp-host",        "TCP server bind address", "host", QString::fromStdString(config.tcpBindHost)},
        {"tcp-port",        "TCP server port", "port", QString::number(config.tcpPort)},
        {"db-host",         "PostgreSQL host", "host", QString::fromStdString(config.dbHost)},
        {"db-port",         "PostgreSQL port", "port", QString::number(config.dbPort)},
        {"db-name",         "Database name", "name", QString::fromStdString(config.dbName)},
        {"db-user",         "Database user", "user", QString::fromStdString(config.dbUser)},
        {"db-password",     "Database password", "pass", QString::fromStdString(config.dbPass)},
        {"pcap-dir",        "Directory for pcap dumps", "dir"},
    });

    parser.process(app);

    // Register meta types
    qRegisterMetaType<DetectionResult>();
    qRegisterMetaType<std::vector<QByteArray>>();

    // List interfaces
    if (parser.isSet("list-interfaces")) {
        AppLogger::get()->info("Available network interfaces:");
        for (const QNetworkInterface& netIface : QNetworkInterface::allInterfaces()) {
            if (netIface.flags().testFlag(QNetworkInterface::IsUp)) {
                QString ipList;
                ipList.reserve(netIface.addressEntries().size() * 17);
                for (const QNetworkAddressEntry& entry : netIface.addressEntries()) {
                    if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                        if (!ipList.isEmpty()) ipList += ", ";
                        ipList += entry.ip().toString();
                    }
                }
                if (!ipList.isEmpty()) {
                    // Используем toUtf8().constData() для корректного вывода через AppLogger (spdlog)
                    AppLogger::get()->info("  \"{}\" (IP: {})", 
                        netIface.humanReadableName().toUtf8().constData(), 
                        ipList.toUtf8().constData());
                }
            }
        }

        AppLogger::get()->info("--- Internal Pcap Interfaces ---");
        auto ifaces = TrafficMonitor::listInterfaces();
        for (auto& [name, desc] : ifaces) {
            AppLogger::get()->info("  {} - {}", name, desc);
        }
        return 0;
    }

    // Validate required args
    if (!parser.isSet("interface") && !parser.isSet("pcap")) {
        AppLogger::get()->error("Either --interface or --pcap must be specified.");
        parser.showHelp(1);
    }

    // Signal handling
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Init detection engine
    DetectionEngine engine;
    std::string modelPath = parser.value("model").toStdString();
    std::string ep = parser.value("ep").toStdString();

    if (!engine.init(modelPath, "", ep)) {
        AppLogger::get()->error("Failed to initialize detection engine.");
        return 1;
    }

    // Init TCP server
    TcpServer tcpServer;
    QString tcpHost = parser.value("tcp-host");
    quint16 tcpPort = parser.value("tcp-port").toUShort();
    if (!tcpServer.startListening(tcpHost, tcpPort)) {
        AppLogger::get()->error("Failed to start TCP server on {} port {}",
            tcpHost.toStdString(), tcpPort);
        return 1;
    }

    // Init database
    DatabaseManager dbManager;
    bool dbConnected = dbManager.connectToDatabase(
        parser.value("db-host"),
        parser.value("db-port").toInt(),
        parser.value("db-name"),
        parser.value("db-user"),
        parser.value("db-password"));

    // Stats counters
    uint64_t totalPackets = 0;
    uint64_t attackCount = 0;
    uint64_t benignCount = 0;

    int sessionId = -1;
    // Stats counters
    uint64_t totalPackets = 0;
    uint64_t attackCount = 0;
    uint64_t benignCount = 0;

    auto createNewSession = [&](const QString& iface, const QString& model) {
        if (dbConnected) {
            if (sessionId > 0) {
                dbManager.closeSession(sessionId, totalPackets, attackCount, benignCount);
                AppLogger::get()->info("Closed previous session ID: {}. Stats: total={}, attacks={}, benign={}",
                                       sessionId, totalPackets, attackCount, benignCount);

                // Reset counters for new session
                totalPackets = 0;
                attackCount = 0;
                benignCount = 0;
            }
            sessionId = dbManager.createSession(iface, model);
            AppLogger::get()->info("Created new session ID: {}", sessionId);
        }
    };

    if (dbConnected) {
        createNewSession(
            parser.isSet("interface") ? parser.value("interface") : "pcap_replay",
            QString::fromStdString(engine.modelInferencer().modelName()));
    }

    auto runReplayMonitor = [&]() {
        QTimer::singleShot(500, [&]() {
            QTimer* checkTimer = new QTimer(&app);
            QObject::connect(checkTimer, &QTimer::timeout, [&, checkTimer]() {
                if (!engine.isRunning()) {
                    AppLogger::get()->info("Replay complete. Sending notification.");
                    auto notifyMsg = Protocol::serializeNotify("replay_done");
                    tcpServer.broadcast(notifyMsg);
                    checkTimer->stop();
                    checkTimer->deleteLater();
                }
            });
            checkTimer->start(1000);
        });
    };

    // Handle incoming commands
    QObject::connect(&tcpServer, &TcpServer::commandReceived, [&](const std::string& cmd, const nlohmann::json& data) {
        if (cmd == Protocol::CMD_LOAD_PCAP) {
            std::string path = data.value("path", "");
            if (!path.empty()) {
                AppLogger::get()->info("Command: load_pcap '{}'", path);
                engine.stop();
                createNewSession(QString::fromStdString(path), 
                                 QString::fromStdString(engine.modelInferencer().modelName()));
                if (engine.startReplay(path)) {
                    runReplayMonitor();
                }
            }
        } else if (cmd == Protocol::CMD_LOAD_MODEL) {
            std::string path = data.value("path", "");
            if (!path.empty()) {
                AppLogger::get()->info("Command: load_model '{}'", path);
                engine.hotSwapModel(path);
            }
        } else if (cmd == Protocol::CMD_CONFIG_BPF) {
            bool enable = data.value("enable", false);
            engine.setMitigationEnabled(enable);
            AppLogger::get()->info("Command: config_bpf (Active Mitigation) enabled={}", enable);
        } else if (cmd == Protocol::CMD_STOP) {
            AppLogger::get()->info("Command: stop");
            g_running = false;
            QCoreApplication::quit();
        }
    });

    // Enable pcap dump if requested
    if (parser.isSet("pcap-dir")) {
        engine.enablePcapDump(parser.value("pcap-dir").toStdString());
    }

    // System metrics
    SystemMetricsCollector metricsCollector;

    // Detection callback
    engine.setResultCallback([&](const DetectionResult& result) {
        DetectionResult r = result;
        r.sessionId = sessionId;
        totalPackets += r.totalPackets;

        if (r.label == 1) attackCount++;
        else benignCount++;

        // Broadcast via TCP
        QByteArray statsMsg = Protocol::serializeStats(r, totalPackets);
        QMetaObject::invokeMethod(&tcpServer, [&tcpServer, statsMsg]() {
            tcpServer.broadcast(statsMsg);
        }, Qt::QueuedConnection);

        // Periodic snapshot
        QByteArray snapMsg = Protocol::serializeSnapshot(
            (float)r.pps, totalPackets, r.label);
        QMetaObject::invokeMethod(&tcpServer, [&tcpServer, snapMsg]() {
            tcpServer.broadcast(snapMsg);
        }, Qt::QueuedConnection);

        // Enqueue to DB
        if (dbConnected) {
            dbManager.enqueueEvent(r);
            dbManager.enqueueSnapshot((float)r.pps, totalPackets, r.label, sessionId);
        }

        // Log
        const char* labelStr = r.label == 1 ? "ATTACK" : "Benign";
        AppLogger::get()->info("[{}] PPS={:.0f} conf={:.3f} total={} tcp={} udp={} icmp={}",
            labelStr, r.pps, r.confidence, r.totalPackets,
            r.tcpPackets, r.udpPackets, r.icmpPackets);
    });

    // Incident callback for DB logging
    engine.setIncidentCallback([&](const QDateTime& start, float dur, const std::string& ip, 
                                   float ppsMax, const std::string& type, float conf) {
        if (dbConnected) {
            dbManager.enqueueSecurityEvent(sessionId, start, dur, 
                QString::fromStdString(ip), ppsMax, QString::fromStdString(type), conf);
        }
    });

    // Start detection
    if (parser.isSet("pcap")) {
        std::string pcapPath = parser.value("pcap").toStdString();
        if (!engine.startReplay(pcapPath)) {
            AppLogger::get()->error("Failed to start pcap replay.");
            return 1;
        }
        runReplayMonitor();
    } else {
        std::string rawIface = parser.value("interface").toStdString();
        std::string iface = resolveNetworkInterface(rawIface);
        if (!engine.startLive(iface)) {
            AppLogger::get()->error("Failed to start live capture on interface: {}", rawIface);
            return 1;
        }
    }

    // Run event loop
    int ret = app.exec();

    // Cleanup
    engine.stop();
    if (dbConnected && sessionId > 0) {
        dbManager.closeSession(sessionId, totalPackets, attackCount, benignCount);
    }

    AppLogger::get()->info("Collector stopped. Total={} Attacks={} Benign={}",
        totalPackets, attackCount, benignCount);
    return ret;
}
