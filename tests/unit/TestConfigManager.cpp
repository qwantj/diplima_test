#include "TestConfigManager.hpp"
#include <QTest>
#include <QTemporaryFile>
#include <QFile>
#include <QTextStream>
#include "common/ConfigManager.hpp"
#include "common/AppLogger.hpp"

void TestConfigManager::initTestCase() {
    // Инициализируем логгер, чтобы избежать потенциальных сбоев при вызове AppLogger::get()
    AppLogger::init();
}

void TestConfigManager::testValidJson() {
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());

    QTextStream out(&tempFile);
    out << R"({
        "collector_host": "192.168.1.10",
        "tcp_port": 12345,
        "database": {
            "host": "db_host",
            "port": 5432,
            "name": "test_db",
            "user": "test_user",
            "password": "test_password"
        },
        "ml": {
            "default_model": "test_model.onnx",
            "default_ep": "gpu",
            "window_sec": 5.0
        },
        "network": {
            "default_interface": "eth0",
            "max_queue_size": 100000
        }
    })";
    tempFile.close();

    AppConfig config;
    bool result = ConfigManager::load(tempFile.fileName().toStdString(), config);

    QVERIFY(result == true);
    QCOMPARE(QString::fromStdString(config.collectorHost), QString("192.168.1.10"));
    QCOMPARE(config.tcpPort, 12345);

    QCOMPARE(QString::fromStdString(config.dbHost), QString("db_host"));
    QCOMPARE(config.dbPort, 5432);
    QCOMPARE(QString::fromStdString(config.dbName), QString("test_db"));
    QCOMPARE(QString::fromStdString(config.dbUser), QString("test_user"));
    QCOMPARE(QString::fromStdString(config.dbPass), QString("test_password"));

    QCOMPARE(QString::fromStdString(config.defaultModel), QString("test_model.onnx"));
    QCOMPARE(QString::fromStdString(config.defaultEp), QString("gpu"));
    QCOMPARE(config.inferenceWindowSec, 5.0);

    QCOMPARE(QString::fromStdString(config.defaultInterface), QString("eth0"));
    QCOMPARE(config.maxQueueSize, 100000);
}

void TestConfigManager::testMissingFile() {
    AppConfig config;
    // Запомним значения по умолчанию для проверки
    std::string defaultHost = config.collectorHost;
    int defaultPort = config.tcpPort;

    bool result = ConfigManager::load("non_existent_file_12345.json", config);

    QVERIFY(result == false);
    QCOMPARE(QString::fromStdString(config.collectorHost), QString::fromStdString(defaultHost));
    QCOMPARE(config.tcpPort, defaultPort);
}

void TestConfigManager::testInvalidJson() {
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());

    QTextStream out(&tempFile);
    // Синтаксическая ошибка в JSON: отсутствует закрывающая кавычка
    out << R"({
        "collector_host": "192.168.1.10,
        "tcp_port": 12345
    })";
    tempFile.close();

    AppConfig config;
    bool result = ConfigManager::load(tempFile.fileName().toStdString(), config);

    QVERIFY(result == false);
}

void TestConfigManager::testPartialJson() {
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());

    QTextStream out(&tempFile);
    out << R"({
        "collector_host": "10.0.0.1"
    })";
    tempFile.close();

    AppConfig config;
    int defaultPort = config.tcpPort; // Должно остаться значением по умолчанию

    bool result = ConfigManager::load(tempFile.fileName().toStdString(), config);

    QVERIFY(result == true);
    QCOMPARE(QString::fromStdString(config.collectorHost), QString("10.0.0.1"));
    QCOMPARE(config.tcpPort, defaultPort); // Не было в JSON, должно остаться дефолтным
}

QTEST_APPLESS_MAIN(TestConfigManager)
