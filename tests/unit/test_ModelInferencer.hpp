#pragma once

#include <QObject>
#include <QtTest/QtTest>

class TestModelInferencer : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testLoadNonExistentModel();
    void testLoadInvalidModel();
};
