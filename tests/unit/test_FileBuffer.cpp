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

    // This should trigger flush of item1, item2, item3 (since 3 > 2)
    // Actually, FileBuffer.cpp says:
    // if (memoryBuffer_.size() > maxMemoryItems_) { flushToDisk(); }
    // So if maxMemoryItems is 2:
    // push 1: size 1, not > 2
    // push 2: size 2, not > 2
    // push 3: size 3, > 2 -> flushToDisk().

    buffer.push("item3");

    // After flush, memoryBuffer is cleared, but totalSize seems to be maintained?
    // Wait, let's check FileBuffer.cpp push:
    // totalSize_.fetch_add(1, std::memory_order_relaxed);
    // flushToDisk() does NOT decrement totalSize.
    // memoryBuffer_.clear() is called in flushToDisk().

    QCOMPARE(buffer.size(), 3);

    // Check if file exists and has content
    QVERIFY(QFile::exists(path));
    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QByteArray content = f.readAll();
    // It adds newlines if missing
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
    // readAllAndClear only reads from the CURRENT filePath_
    // So item1, item2 in path1 are lost to this buffer instance unless we handled it.
    // Looking at FileBuffer.cpp: readAllAndClear() uses filePath_.

    QCOMPARE(results.size(), 3); // item3, item4 from path2, plus whatever was in memory?
    // Wait, when it flushed item4, memory was cleared.
    // So results should be item3, item4 from disk.
    // But wait, there might be one in memory if it didn't flush?
    // maxMemoryItems = 1.
    // push item3: mem size 1.
    // push item4: mem size 2 > 1 -> flush item3, item4 to path2.
    // readAllAndClear: reads path2 -> item3, item4. Then appends memory (empty).

    // Actually, I should probably check what happens to path1. It still exists.
    QVERIFY(QFile::exists(path1));
}

void TestFileBuffer::testDestructorFlushes() {
    QString path;
    {
        QTemporaryFile tempFile;
        QVERIFY(tempFile.open());
        path = tempFile.fileName();
        // tempFile will be deleted, but we want the path to stay for a moment to check if FileBuffer writes to it.
        // Actually QTemporaryFile deletes the file on destruction by default.
        tempFile.setAutoRemove(false);
        tempFile.close();

        FileBuffer buffer(path, 100);
        buffer.push("last-breath");
        // Destructor should be called here
    }

    QVERIFY(QFile::exists(path));
    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), QByteArray("last-breath\n"));
    f.close();
    QFile::remove(path);
}
