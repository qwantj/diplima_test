#include <QtTest>
#include "common/Protocol.hpp"

class TestProtocol : public QObject {
    Q_OBJECT
private slots:
    void testFrame() {
        QByteArray payload = "hello";
        QByteArray framed = Protocol::frame(payload);
        QCOMPARE(framed, QByteArray("hello\n"));
    }
};

QTEST_MAIN(TestProtocol)
#include "test_protocol.moc"
