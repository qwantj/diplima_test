#include <QCoreApplication>
#include <QtTest>

#include "test_ConfigManager.hpp"
#include "ProtocolTests.hpp"
#include "test_FileBuffer.hpp"
#include "CSVUtilsTests.hpp"
#include "test_ModelInferencer.hpp"
#include "AppLoggerTests.hpp"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    int status = 0;

    TestConfigManager configTest;
    status |= QTest::qExec(&configTest, argc, argv);

    ProtocolTests protocolTest;
    status |= QTest::qExec(&protocolTest, argc, argv);

    TestFileBuffer fileBufferTest;
    status |= QTest::qExec(&fileBufferTest, argc, argv);

    CSVUtilsTests csvTest;
    status |= QTest::qExec(&csvTest, argc, argv);

    AppLoggerTests loggerTest;
    status |= QTest::qExec(&loggerTest, argc, argv);

    TestModelInferencer modelTest;
    status |= QTest::qExec(&modelTest, argc, argv);

    return status;
}
