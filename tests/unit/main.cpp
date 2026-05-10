#include <QTest>
#include <QCoreApplication>
#include <iostream>

#include "ProtocolTests.hpp"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    int status = 0;

    {
        ProtocolTests protocolTests;
        status |= QTest::qExec(&protocolTests, argc, argv);
    }

    return status;
}
