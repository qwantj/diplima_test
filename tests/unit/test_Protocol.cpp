#include "test_Protocol.hpp"
#include "common/Protocol.hpp"
#include <nlohmann/json.hpp>

void TestProtocol::testParseValidMessage() {
        QByteArray validJson = R"({
            "version": "2.2",
            "type": "stats",
            "total_packets": 100,
            "data": {
                "label": 1,
                "confidence": 0.95
            }
        })";

        auto [type, payload] = Protocol::parseMessage(validJson);

        QCOMPARE(QString::fromStdString(type), QString("stats"));
        QVERIFY(payload.contains("version"));
        QCOMPARE(QString::fromStdString(payload["version"].get<std::string>()), QString("2.2"));
        QCOMPARE(payload["total_packets"].get<int>(), 100);
        QVERIFY(payload.contains("data"));
        QCOMPARE(payload["data"]["label"].get<int>(), 1);
    }

void TestProtocol::testParseInvalidJson() {
        QByteArray invalidJson = "{ invalid json data, ; ] }";

        auto [type, payload] = Protocol::parseMessage(invalidJson);

        QCOMPARE(QString::fromStdString(type), QString("error"));
        QVERIFY(payload.empty());
    }

void TestProtocol::testParseMissingType() {
        QByteArray missingTypeJson = R"({
            "version": "2.2",
            "data": "some data"
        })";

        auto [type, payload] = Protocol::parseMessage(missingTypeJson);

        QCOMPARE(QString::fromStdString(type), QString(""));
        QVERIFY(payload.contains("data"));
    }

void TestProtocol::testSerializeResult() {
        DetectionResult result;
        result.label = 1;
        result.confidence = 0.88f;
        result.pps = 100.5;
        result.totalPackets = 500;
        result.modelName = "test_model";
        result.sessionId = 42;

        nlohmann::json j = Protocol::serializeResult(result);

        QVERIFY(j.contains("version"));
        QCOMPARE(QString::fromStdString(j["version"].get<std::string>()), QString(Protocol::PROTOCOL_VERSION.c_str()));

        QVERIFY(j.contains("label"));
        QCOMPARE(j["label"].get<int>(), 1);

        QVERIFY(j.contains("confidence"));
        // Floating point comparison with a small epsilon
        QVERIFY(std::abs(j["confidence"].get<float>() - 0.88f) < 0.001f);

        QVERIFY(j.contains("pps"));
        QVERIFY(std::abs(j["pps"].get<double>() - 100.5) < 0.001);

        QVERIFY(j.contains("total_packets"));
        QCOMPARE(j["total_packets"].get<uint64_t>(), 500);

        QVERIFY(j.contains("model"));
        QCOMPARE(QString::fromStdString(j["model"].get<std::string>()), QString("test_model"));

        QVERIFY(j.contains("session_id"));
        QCOMPARE(j["session_id"].get<int>(), 42);
    }

void TestProtocol::testDeserializeResult() {
        nlohmann::json j = {
            {"label", 1},
            {"confidence", 0.99},
            {"pps", 1500.0},
            {"total_packets", 10000},
            {"model", "rf_model"},
            {"session_id", 10}
        };

        DetectionResult result = Protocol::deserializeResult(j);

        QCOMPARE(result.label, 1);
        QVERIFY(std::abs(result.confidence - 0.99f) < 0.001f);
        QVERIFY(std::abs(result.pps - 1500.0) < 0.001);
        QCOMPARE(result.totalPackets, 10000);
        QCOMPARE(QString::fromStdString(result.modelName), QString("rf_model"));
        QCOMPARE(result.sessionId, 10);
}
