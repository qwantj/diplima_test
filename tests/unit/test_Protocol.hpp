#pragma once
#include <QtTest>

class TestProtocol : public QObject {
    Q_OBJECT
private slots:
    void testParseValidMessage();
    void testParseInvalidJson();
    void testParseMissingType();
    void testSerializeResult();
    void testDeserializeResult();
};
