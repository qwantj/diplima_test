#include "test_ConfigManager.hpp"
#include <QTemporaryFile>
#include "common/ConfigManager.hpp"
#include "common/AppLogger.hpp"
#include <fstream>
#include <QFile>

void TestConfigManager::initTestCase() {
    AppLogger::init();
}

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
        QCOMPARE(QString::fromStdString(config.collectorHost), QString("127.0.0.1"));
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
        // Set non-default values to verify they are retained on failure
        config.collectorHost = "existing_host";
        config.tcpPort = 9999;

        bool result = ConfigManager::load(filePath.toStdString(), config);

        QFile::remove(filePath);

        QVERIFY(!result);
        // Values should remain unchanged if load fails
        QCOMPARE(QString::fromStdString(config.collectorHost), QString("existing_host"));
        QCOMPARE(config.tcpPort, 9999);
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

void TestConfigManager::testLoadEmptyFile() {
    QString filePath;
    {
        QTemporaryFile tempFile;
        tempFile.setAutoRemove(false);
        QVERIFY(tempFile.open());
        filePath = tempFile.fileName();
        tempFile.close();
    }

    AppConfig config;
    config.collectorHost = "some_host";

    bool result = ConfigManager::load(filePath.toStdString(), config);

    QFile::remove(filePath);

    // Empty file is typically invalid for nlohmann::json if it expects an object
    QVERIFY(!result);
    QCOMPARE(QString::fromStdString(config.collectorHost), QString("some_host"));
}

void TestConfigManager::testSaveDoesNotPersistPassword() {
    QString filePath;
    {
        QTemporaryFile tempFile;
        tempFile.setAutoRemove(false);
        QVERIFY(tempFile.open());
        filePath = tempFile.fileName();
        tempFile.close();
    }

    AppConfig config;
    config.dbPass = "secret_password";
    config.dbUser = "test_user";

    QVERIFY(ConfigManager::save(filePath.toStdString(), config));

    AppConfig loadedConfig;
    QVERIFY(ConfigManager::load(filePath.toStdString(), loadedConfig));

    QFile::remove(filePath);

    QCOMPARE(QString::fromStdString(loadedConfig.dbUser), QString("test_user"));
    // Password should NOT be persisted and thus should be empty when loaded back
    QCOMPARE(QString::fromStdString(loadedConfig.dbPass), QString(""));
}

void TestConfigManager::testEnvOverride() {
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString filePath = tempFile.fileName();
    tempFile.close();

    AppConfig config;
    config.dbUser = "json_user";
    config.dbPass = "json_pass";
    QVERIFY(ConfigManager::save(filePath.toStdString(), config));

#ifdef Q_OS_WIN
    _putenv("DDOS_DB_USER=env_user");
    _putenv("DDOS_DB_PASS=env_pass");
#else
    setenv("DDOS_DB_USER", "env_user", 1);
    setenv("DDOS_DB_PASS", "env_pass", 1);
#endif

    AppConfig loadedConfig;
    QVERIFY(ConfigManager::load(filePath.toStdString(), loadedConfig));

    QCOMPARE(QString::fromStdString(loadedConfig.dbUser), QString("env_user"));
    QCOMPARE(QString::fromStdString(loadedConfig.dbPass), QString("env_pass"));

    // Cleanup environment to avoid affecting other tests
#ifdef Q_OS_WIN
    _putenv("DDOS_DB_USER=");
    _putenv("DDOS_DB_PASS=");
#else
    unsetenv("DDOS_DB_USER");
    unsetenv("DDOS_DB_PASS");
#endif
}
