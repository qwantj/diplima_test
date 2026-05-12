#pragma once
#include <QtTest>

class TestConfigManager : public QObject {
    Q_OBJECT
private slots:
    void testLoadValidJson();
    void testLoadMissingFile();
    void testLoadInvalidJson();
    void testLoadPartialJson();
    void testSaveDoesNotPersistPassword();
};
