#pragma once

#include <QByteArray>
#include <QFile>
#include <QMutex>
#include <QString>
#include <deque>
#include <vector>
#include <atomic>

class FileBuffer {
public:
    explicit FileBuffer(const QString& filePath = "", size_t maxMemoryItems = 10000);
    ~FileBuffer();

    void setFilePath(const QString& path);
    void push(const QByteArray& data);
    std::vector<QByteArray> readAllAndClear();
    void clear();
    int size() const;

private:
    void flushToDisk();

    QString filePath_;
    size_t maxMemoryItems_;
    std::deque<QByteArray> memoryBuffer_;
    mutable QMutex mutex_;
    std::atomic<int> totalSize_{0};
};
