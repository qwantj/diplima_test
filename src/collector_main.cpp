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

static std::atomic<bool> g_running{true};

static void signalHandler(int) {
    g_running = false;
    QCoreApplication::quit();
}

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

int main(int argc, char* argv[]) {
#ifdef Q_OS_WIN
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    QCoreApplication app(argc, argv);
    app.setApplicationName("ddos_collector");
    app.setApplicationVersion("2.2");

    AppLogger::init("ddos_collector.log");

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

    qRegisterMetaType<DetectionResult>();
    qRegisterMetaType<std::vector<QByteArray>>();

    if (parser.isSet("list-interfaces")) {
        AppLogger::get()->info("Available network interfaces:");
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

    if (!parser.isSet("interface") && !parser.isSet("pcap")) {
        AppLogger::get()->error("Either --interface or --pcap must be specified.");
        parser.showHelp(1);
    }

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    DetectionEngine engine;
    std::string modelPath = parser.value("model").toStdString();
    std::string ep = parser.value("ep").toStdString();

    if (!engine.init(modelPath, "", ep)) {
        AppLogger::get()->error("Failed to initialize detection engine.");
        return 1;
    }

    TcpServer tcpServer;
    QString tcpHost = parser.value("tcp-host");
    quint16 tcpPort = parser.value("tcp-port").toUShort();
    if (!tcpServer.startListening(tcpHost, tcpPort)) {
        AppLogger::get()->error("Failed to start TCP server on {} port {}",
            tcpHost.toStdString(), tcpPort);
        return 1;
    }

    DatabaseManager dbManager;
    bool dbConnected = dbManager.connectToDatabase(
        parser.value("db-host"),
        parser.value("db-port").toInt(),
        parser.value("db-name"),
        parser.value("db-user"),
        parser.value("db-password"));

    uint64_t totalPackets = 0;
    uint64_t attackCount = 0;
    uint64_t benignCount = 0;

    int sessionId = -1;
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

    // PCAP Replay State
    DetectionEngine* replayEngine = nullptr;
    std::atomic<bool> isReplaying{false};
    int replaySessionId = -1;
    std::atomic<uint64_t> replayTotalPackets{0};

    // Incident aggregation state
    bool inAttack = false;
    QDateTime attackStartTime;
    float maxPpsSeen = 0;
    float totalConf = 0;
    int samples = 0;
    std::string attacker = "unknown";

    auto processResultForDb = [&](const DetectionResult& r, int sid) {
        if (!dbConnected || sid <= 0) return;
        
        dbManager.enqueueEvent(r);
        dbManager.enqueueSnapshot((float)r.pps, totalPackets, r.label, sid);

        // Simple aggregation logic for security incidents
        if (r.label == 1 && r.confidence > 0.4) {
            if (!inAttack) {
                inAttack = true;
                attackStartTime = r.timestamp.isValid() ? r.timestamp : (QDateTime::currentDateTime)();
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
            // Attack ended or dip in confidence
            float duration = attackStartTime.secsTo(r.timestamp.isValid() ? r.timestamp : (QDateTime::currentDateTime)());
            if (duration <= 0) duration = (float)samples; 
            
            dbManager.enqueueSecurityEvent(sid, attackStartTime, duration,
                QString::fromStdString(attacker), maxPpsSeen,
                QString::fromStdString(r.modelName), totalConf / samples);
            inAttack = false;
        }
    };

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
                AppLogger::get()->info("Replay started for: {}", path);
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
        } else if (cmd == Protocol::CMD_CONFIG_DUMP) {
            bool enable = data.value("enable", false);
            if (enable) engine.enablePcapDump("pcap_dumps");
            else engine.disablePcapDump();
        } else if (cmd == Protocol::CMD_STOP) {
            g_running = false;
            QCoreApplication::quit();
        }
    });

    engine.setResultCallback([&](const DetectionResult& result) {
        DetectionResult r = result;
        r.sessionId = sessionId;
        totalPackets += r.totalPackets;
        if (r.label == 1) attackCount++; else benignCount++;

        if (!isReplaying) {
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

    if (parser.isSet("pcap")) {
        engine.startReplay(parser.value("pcap").toStdString());
    } else {
        std::string iface = resolveNetworkInterface(parser.value("interface").toStdString());
        engine.startLive(iface);
    }

    int ret = app.exec();
    engine.stop();
    if (replayEngine) { replayEngine->stop(); delete replayEngine; }
    if (dbConnected && sessionId > 0) dbManager.closeSession(sessionId, totalPackets, attackCount, benignCount);
    return ret;
}
