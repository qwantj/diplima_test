#include <QtTest>
#include "common/Protocol.hpp"

class TestProtocol : public QObject {
    Q_OBJECT
private slots:
    void testStatsSerialization() {
        DetectionResult r;
        r.sessionId = 42;
        r.pps = 1000.5;
        r.label = 1;
        r.confidence = 0.95f;
        r.totalPackets = 50000;

        QByteArray data = Protocol::serializeStats(r, 60000);
        QVERIFY(!data.isEmpty());

        // Check if it's valid JSON
        try {
            auto j = nlohmann::json::parse(data.begin(), data.end());
            QCOMPARE(QString::fromStdString(j["cmd"]), QString(Protocol::CMD_STATS));
            QCOMPARE(j["data"]["session_id"].get<int>(), 42);
            QCOMPARE(j["data"]["pps"].get<double>(), 1000.5);
            QCOMPARE(j["data"]["total_packets"].get<uint64_t>(), 60000);
        } catch (...) {
            QFAIL("Failed to parse serialized JSON");
        }
    }

    void testErrorHandling() {
        // Test parsing invalid JSON
        std::string cmd;
        nlohmann::json data;
        QVERIFY(!Protocol::parseCommand("invalid json", cmd, data));
    }
};

#include "test_protocol.moc"
