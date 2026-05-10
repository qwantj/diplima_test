#include "ProtocolTests.hpp"
#include <string>

void ProtocolTests::testParseValidMessage() {
    QByteArray validJson = "{\"type\":\"test_msg\",\"data\":123}";
    auto result = Protocol::parseMessage(validJson);

    QCOMPARE(result.first, std::string("test_msg"));
    QCOMPARE(result.second.value("data", 0), 123);
}

void ProtocolTests::testParseInvalidJson() {
    QByteArray invalidJson = "{ invalid_json: true, ]";
    auto result = Protocol::parseMessage(invalidJson);

    QCOMPARE(result.first, std::string("error"));
    QVERIFY(result.second.empty());
}

void ProtocolTests::testParseEmptyMessage() {
    QByteArray emptyMessage = "";
    auto result = Protocol::parseMessage(emptyMessage);

    QCOMPARE(result.first, std::string("error"));
    QVERIFY(result.second.empty());
}

// Ensure moc can see this test class.
// Since we don't have automoc in the unit test main setup by default or it's handled globally,
// we just let CMake's CMAKE_AUTOMOC handle it, which requires no explicit include here
// since the class is defined in ProtocolTests.hpp.
