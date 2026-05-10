#include <QtTest>
#include "test_config.cpp"
#include "test_protocol.cpp"

int main(int argc, char** argv) {
    int status = 0;
    {
        TestConfig tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestProtocol tp;
        status |= QTest::qExec(&tp, argc, argv);
    }
    return status;
}
