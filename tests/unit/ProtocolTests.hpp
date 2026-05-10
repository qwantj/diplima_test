#pragma once

#include <QObject>
#include <QTest>
#include "common/Protocol.hpp"

class ProtocolTests : public QObject {
    Q_OBJECT

private slots:
    void testParseValidMessage();
    void testParseInvalidJson();
    void testParseEmptyMessage();
};
