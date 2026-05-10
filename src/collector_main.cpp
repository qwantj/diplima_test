#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTimer>

#include "common/AppLogger.hpp"
#include "common/Protocol.hpp"
#include "common/TcpServer.hpp"
#include "common/DatabaseManager.hpp"
#include "common/SystemMetricsCollector.hpp"
#include "core/DetectionEngine.hpp"

#include <csignal>
#include <atomic>
#include <QNetworkInterface>
#include <QHostAddress>

static std::atomic<bool> g_running{true};

static void signalHandler(int) {
    g_running = false;
    QCoreApplication::quit();
}

// Поиск IP-адреса по человекочитаемому имени (например, "Wi-Fi" или "Ethernet")
static std::string resolveNetworkInterface(const std::string& userInput) {
    QString input = QString::fromStdString(userInput);
    for (const QNetworkInterface& netIface : QNetworkInterface::allInterfaces()) {
        if (netIface.humanReadableName().compare(input, Qt::CaseInsensitive) == 0 ||
            netIface.name().compare(input, Qt::CaseInsensitive) == 0) {
            for (const QNetworkAddressEntry& entry : netIface.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                    std::string ipStr = entry.ip().toString().toStdString();
                    AppLogger::get()->info("Resolved alias '{}' to IP: {}", userInput, ipStr);
                    return ipStr;
                }
            }
        }
    }
    return userInput; // Если не найдено, возвращаем как есть (может быть это уже IP)
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("ddos_collector");
    app.setApplicationVersion("2.2");

    QCommandLineParser parser;
    parser.setApplicationDescription("DDoS Detection Collector");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOptions({
        {{"i", "interface"}, "Network interface name", "name"},
        {{"f", "pcap"},     "Pcap file to replay", "file"},
        {{"m", "model"},    "ONNX model path", "path", "models/rf_model.onnx"},
        {{"e", "ep"},       "Execution provider (cpu|cuda|dml)", "ep", "cpu"},
        {"list-interfaces", "List available network interfaces"},
        {"tcp-port",        "TCP server port", "port", "50050"},
        {"db-host",         "PostgreSQL host", "host", "localhost"},
        {"db-port",         "PostgreSQL port", "port", "5432"},
        {"db-name",         "Database name", "name", "ddos_detection_db"},
        {"db-user",         "Database user", "user", "postgres"},
        {"db-password",     "Database password", "pass", "qwerty"},
        {"pcap-dir",        "Directory for pcap dumps", "dir"},
    });

    parser.process(app);

    // Init logging
    AppLogger::init("ddos_collector.log");

    // Register meta types
    qRegisterMetaType<DetectionResult>();
    qRegisterMetaType<std::vector<QByteArray>>();

    // List interfaces
    if (parser.isSet("list-interfaces")) {
        AppLogger::get()->info("Available user-friendly network interfaces:");
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
                        netIface.humanReadableName().toStdString(), ipList.toStdString());
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
    quint16 tcpPort = parser.value("tcp-port").toUShort();
    if (!tcpServer.startListening(tcpPort)) {
        AppLogger::get()->error("Failed to start TCP server on port {}", tcpPort);
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

    int sessionId = -1;
    if (dbConnected) {
        sessionId = dbManager.createSession(
            parser.isSet("interface") ? parser.value("interface") : "pcap_replay",
            QString::fromStdString(engine.modelInferencer().modelName()));
    }

    // Enable pcap dump if requested
    if (parser.isSet("pcap-dir")) {
        engine.enablePcapDump(parser.value("pcap-dir").toStdString());
    }

    // System metrics
    SystemMetricsCollector metricsCollector;

    // Stats counters
    uint64_t totalPackets = 0;
    uint64_t attackCount = 0;
    uint64_t benignCount = 0;

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

    // Start detection
    if (parser.isSet("pcap")) {
        std::string pcapPath = parser.value("pcap").toStdString();
        if (!engine.startReplay(pcapPath)) {
            AppLogger::get()->error("Failed to start pcap replay.");
            return 1;
        }

        // Wait for replay to finish
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
