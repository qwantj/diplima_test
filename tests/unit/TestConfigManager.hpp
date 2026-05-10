#pragma once

#include <QObject>

class TestConfigManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testValidJson();
    void testMissingFile();
    void testInvalidJson();
    void testPartialJson();
};
