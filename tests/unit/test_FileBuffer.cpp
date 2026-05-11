#include "test_FileBuffer.hpp"
#include <QTemporaryFile>
#include <QFile>
#include <QFileInfo>

void TestFileBuffer::testPushAndSize() {
    FileBuffer buffer("", 10);
    QCOMPARE(buffer.size(), 0);

    buffer.push("item1");
    QCOMPARE(buffer.size(), 1);

    buffer.push("item2");
    QCOMPARE(buffer.size(), 2);
}

void TestFileBuffer::testReadAllAndClearInMemory() {
    FileBuffer buffer("", 10);
    buffer.push("item1");
    buffer.push("item2");

    auto results = buffer.readAllAndClear();
    QCOMPARE(results.size(), 2);
    QCOMPARE(results[0], QByteArray("item1"));
    QCOMPARE(results[1], QByteArray("item2"));
    QCOMPARE(buffer.size(), 0);
}

void TestFileBuffer::testFlushToDisk() {
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString path = tempFile.fileName();
    tempFile.close();

    // Small limit to trigger flush
    FileBuffer buffer(path, 2);
    buffer.push("item1");
    buffer.push("item2");
    QCOMPARE(buffer.size(), 2);

    buffer.push("item3");

    QCOMPARE(buffer.size(), 3);

    // Check if file exists and has content
    QVERIFY(QFile::exists(path));
    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QByteArray content = f.readAll();
    QCOMPARE(content, QByteArray("item1\nitem2\nitem3\n"));
    f.close();
}

void TestFileBuffer::testReadAllAndClearMixed() {
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString path = tempFile.fileName();
    tempFile.close();

    FileBuffer buffer(path, 2);
    buffer.push("disk1");
    buffer.push("disk2");
    buffer.push("disk3"); // Triggers flush of 1, 2, 3

    buffer.push("mem1");

    QCOMPARE(buffer.size(), 4);

    auto results = buffer.readAllAndClear();
    QCOMPARE(results.size(), 4);
    QCOMPARE(results[0], QByteArray("disk1"));
    QCOMPARE(results[1], QByteArray("disk2"));
    QCOMPARE(results[2], QByteArray("disk3"));
    QCOMPARE(results[3], QByteArray("mem1"));
    QCOMPARE(buffer.size(), 0);

    // File should be removed after readAllAndClear
    QVERIFY(!QFile::exists(path));
}

void TestFileBuffer::testClear() {
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString path = tempFile.fileName();
    tempFile.close();

    FileBuffer buffer(path, 2);
    buffer.push("item1");
    buffer.push("item2");
    buffer.push("item3"); // Flush
    buffer.push("item4");

    QCOMPARE(buffer.size(), 4);
    QVERIFY(QFile::exists(path));

    buffer.clear();
    QCOMPARE(buffer.size(), 0);
    QVERIFY(!QFile::exists(path));

    auto results = buffer.readAllAndClear();
    QCOMPARE(results.size(), 0);
}

void TestFileBuffer::testFilePathUpdate() {
    QTemporaryFile tempFile1;
    QVERIFY(tempFile1.open());
    QString path1 = tempFile1.fileName();
    tempFile1.close();

    FileBuffer buffer(path1, 1);
    buffer.push("item1");
    buffer.push("item2"); // Flush to path1
    QVERIFY(QFile::exists(path1));

    QTemporaryFile tempFile2;
    QVERIFY(tempFile2.open());
    QString path2 = tempFile2.fileName();
    tempFile2.close();

    buffer.setFilePath(path2);
    buffer.push("item3");
    buffer.push("item4"); // Flush to path2
    QVERIFY(QFile::exists(path2));

    auto results = buffer.readAllAndClear();
    QCOMPARE(results.size(), 3);
    QVERIFY(QFile::exists(path1));
}

void TestFileBuffer::testDestructorFlushes() {
    QString path;
    {
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        path = tempFile.fileName();
        tempFile.setAutoRemove(false);
        tempFile.close();

        FileBuffer buffer(path, 100);
        buffer.push("last-breath");
    }

    QVERIFY(QFile::exists(path));
    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), QByteArray("last-breath\n"));
    f.close();
    QFile::remove(path);
}
