#pragma once
#include <QtTest>

class TestConfigManager : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void testLoadValidJson();
    void testLoadMissingFile();
    void testLoadInvalidJson();
    void testLoadPartialJson();
    void testLoadEmptyFile();
    void testEnvOverride();
    void testSaveDoesNotPersistPassword();
};
