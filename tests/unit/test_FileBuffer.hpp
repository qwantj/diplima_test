#pragma once

#include <QObject>
#include <QtTest>
#include "common/FileBuffer.hpp"

class TestFileBuffer : public QObject {
    Q_OBJECT

private slots:
    void testPushAndSize();
    void testReadAllAndClearInMemory();
    void testFlushToDisk();
    void testReadAllAndClearMixed();
    void testClear();
    void testFilePathUpdate();
    void testDestructorFlushes();
};
