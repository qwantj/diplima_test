#pragma once

#include <QByteArray>
#include <QFile>
#include <QMutex>
#include <QString>
#include <deque>
#include <vector>
#include <atomic>
#include "concurrentqueue.h"

class FileBuffer {
public:
    explicit FileBuffer(const QString& filePath = "");

    void setFilePath(const QString& path);
    void append(const QByteArray& data);
    std::deque<QByteArray> readAll();
    void clear();
    int  size() const;

private:
    QString filePath_;
    moodycamel::ConcurrentQueue<QByteArray> buffer_;
    std::atomic<int> size_{0};
    mutable QMutex fileMutex_; // Only for file writing, memory buffer is lock-free
};
