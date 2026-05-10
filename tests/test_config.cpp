#include <QtTest>
#include "common/ConfigManager.hpp"
#include <filesystem>
#include <fstream>

class TestConfig : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        std::ofstream f("test_config.json");
        f << "{\"collector_host\": \"127.0.0.1\", \"tcp_port\": 12345}";
        f.close();
    }

    void testLoad() {
        AppConfig config;
        QVERIFY(ConfigManager::load("test_config.json", config));
        QCOMPARE(QString::fromStdString(config.collectorHost), QString("127.0.0.1"));
        QCOMPARE(config.tcpPort, (quint16)12345);
    }

    void cleanupTestCase() {
        std::filesystem::remove("test_config.json");
    }
};

QTEST_MAIN(TestConfig)
#include "test_config.moc"
