#include "test_Protocol.hpp"
#include "common/Protocol.hpp"

void TestProtocol::testParseValidMessage() {
    QByteArray validJson = R"({
        "type": "stats",
        "total_packets": 100,
        "session_id": 42,
        "pps": 100.5,
        "label": 1,
        "confidence": 0.95
    })";

    auto [type, payload] = Protocol::parseMessage(validJson);

    QCOMPARE(QString::fromStdString(type), QString("stats"));
    QCOMPARE(payload["session_id"].get<int>(), 42);
    QCOMPARE(payload["total_packets"].get<int>(), 100);
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

    QVERIFY(j.contains("label"));
    QCOMPARE(j["label"].get<int>(), 1);
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
    QCOMPARE(result.sessionId, 10);
}
