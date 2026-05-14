#pragma once

#include <QtTest>

class TestSystemMetricsCollector : public QObject {
    Q_OBJECT

private slots:
    void testValidData();
    void testConsistency();
};
