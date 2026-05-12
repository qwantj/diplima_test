#include <QtTest>
#include <QTemporaryFile>
#include "common/ConfigManager.hpp"

class TestConfig : public QObject {
    Q_OBJECT
private slots:
    void testLoadSave() {
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        QString filePath = tempFile.fileName();
        tempFile.close();

        AppConfig config;
        config.dbHost = "test_host";
        config.dbPort = 1234;
        config.dbUser = "test_user";
        config.dbPass = "test_pass";

        QVERIFY(ConfigManager::save(filePath.toStdString(), config));

        AppConfig loadedConfig;
        QVERIFY(ConfigManager::load(filePath.toStdString(), loadedConfig));

        QCOMPARE(QString::fromStdString(loadedConfig.dbHost), QString("test_host"));
        QCOMPARE(loadedConfig.dbPort, 1234);
        QCOMPARE(QString::fromStdString(loadedConfig.dbUser), QString("test_user"));
        // Password should NOT be persisted and thus should be empty when loaded back
        QCOMPARE(QString::fromStdString(loadedConfig.dbPass), QString(""));
    }

    void testEnvOverride() {
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
    }
};

#include "test_config.moc"
