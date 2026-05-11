#include "test_ConfigManager.hpp"
#include <QTemporaryFile>
#include "common/ConfigManager.hpp"
#include <fstream>
#include <QFile>

void TestConfigManager::testLoadValidJson() {
        QString filePath;
        {
            QTemporaryFile tempFile;
            tempFile.setAutoRemove(false);
            QVERIFY(tempFile.open());
            filePath = tempFile.fileName();
            tempFile.close();
        }

        {
            QFile f(filePath);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(R"({
                "collector_host": "192.168.1.10",
                "tcp_port": 12345,
                "database": {
                    "host": "db.local",
                    "port": 5433,
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
                    "max_queue_size": 1000
                }
            })");
            f.close();
        }

        AppConfig config;
        bool result = ConfigManager::load(filePath.toStdString(), config);

        QFile::remove(filePath);

        QVERIFY(result);
        QCOMPARE(QString::fromStdString(config.collectorHost), QString("192.168.1.10"));
        QCOMPARE(config.tcpPort, 12345);

        QCOMPARE(QString::fromStdString(config.dbHost), QString("db.local"));
        QCOMPARE(config.dbPort, 5433);
        QCOMPARE(QString::fromStdString(config.dbName), QString("test_db"));
        QCOMPARE(QString::fromStdString(config.dbUser), QString("test_user"));
        QCOMPARE(QString::fromStdString(config.dbPass), QString("test_password"));

        QCOMPARE(QString::fromStdString(config.defaultModel), QString("test_model.onnx"));
        QCOMPARE(QString::fromStdString(config.defaultEp), QString("gpu"));
        QCOMPARE(config.inferenceWindowSec, 5.0);

        QCOMPARE(QString::fromStdString(config.defaultInterface), QString("eth0"));
        QCOMPARE(config.maxQueueSize, 1000);
    }

void TestConfigManager::testLoadMissingFile() {
        AppConfig config;
        std::string missingFile = "non_existent_config.json";

        // Remove file if it exists just to be sure
        std::remove(missingFile.c_str());

        bool result = ConfigManager::load(missingFile, config);

        QVERIFY(!result);
        // Default values should be untouched
        QCOMPARE(QString::fromStdString(config.collectorHost), QString("localhost"));
        QCOMPARE(config.tcpPort, 50050);
    }

void TestConfigManager::testLoadInvalidJson() {
        QString filePath;
        {
            QTemporaryFile tempFile;
            tempFile.setAutoRemove(false);
            QVERIFY(tempFile.open());
            filePath = tempFile.fileName();
            tempFile.close();
        }

        {
            QFile f(filePath);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(R"({
                "collector_host": "192.168.1.10",
                "tcp_port": 12345,
                INVALID JSON HERE
            })");
            f.close();
        }

        AppConfig config;
        bool result = ConfigManager::load(filePath.toStdString(), config);

        QFile::remove(filePath);

        QVERIFY(!result);
        // Values should remain as defaults since load returns false on exception
        QCOMPARE(QString::fromStdString(config.collectorHost), QString("localhost"));
        QCOMPARE(config.tcpPort, 50050);
    }

void TestConfigManager::testLoadPartialJson() {
        QString filePath;
        {
            QTemporaryFile tempFile;
            tempFile.setAutoRemove(false);
            QVERIFY(tempFile.open());
            filePath = tempFile.fileName();
            tempFile.close();
        }

        {
            QFile f(filePath);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(R"({
                "tcp_port": 8080
            })");
            f.close();
        }

        AppConfig config;
        // Pre-modify a default to check it stays
        config.collectorHost = "custom_host";

        bool result = ConfigManager::load(filePath.toStdString(), config);

        QFile::remove(filePath);

        QVERIFY(result);
        // Modified values
        QCOMPARE(config.tcpPort, 8080);
        // Untouched defaults/current values
        QCOMPARE(QString::fromStdString(config.collectorHost), QString("custom_host"));
        QCOMPARE(QString::fromStdString(config.dbHost), QString("localhost"));
}
