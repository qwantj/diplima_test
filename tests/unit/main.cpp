#include <QCoreApplication>
#include <QtTest>

#include "test_ConfigManager.hpp"
#include "test_Protocol.hpp"
#include "test_FileBuffer.hpp"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    int status = 0;

    TestConfigManager configTest;
    status |= QTest::qExec(&configTest, argc, argv);

    TestProtocol protocolTest;
    status |= QTest::qExec(&protocolTest, argc, argv);

    TestFileBuffer fileBufferTest;
    status |= QTest::qExec(&fileBufferTest, argc, argv);

    return status;
}
