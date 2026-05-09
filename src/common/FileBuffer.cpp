#include "common/FileBuffer.hpp"
#include <deque>

FileBuffer::FileBuffer(const QString& filePath) : filePath_(filePath) {}

void FileBuffer::setFilePath(const QString& path) { filePath_ = path; }

void FileBuffer::append(const QByteArray& data) {
    buffer_.enqueue(data);
    size_.fetch_add(1, std::memory_order_relaxed);

    if (!filePath_.isEmpty()) {
        QMutexLocker lock(&fileMutex_);
        QFile f(filePath_);
        if (f.open(QIODevice::Append)) {
            f.write(data);
            f.write("\n");
            f.close();
        }
    }
}

std::deque<QByteArray> FileBuffer::readAll() {
    std::deque<QByteArray> result;
    // Note: Since this is a lock-free queue, there is no iterator to just copy
    // We maintain original semantics where we want a snapshot, but since
    // it's a ConcurrentQueue without destructive copy, we consume and re-enqueue
    QByteArray item;
    int count = 0;
    while (buffer_.try_dequeue(item)) {
        result.push_back(item);
        count++;
    }
    size_.fetch_sub(count, std::memory_order_relaxed);

    // To mimic original non-destructive behavior (which is rare for a true buffer)
    // We enqueue them back. In a high-perf system this is actually
    // generally undesired (reading should consume), but we do it for parity
    for (const auto& i : result) {
        buffer_.enqueue(i);
    }
    size_.fetch_add(count, std::memory_order_relaxed);

    return result;
}

void FileBuffer::clear() {
    QByteArray item;
    int count = 0;
    while (buffer_.try_dequeue(item)) {
        count++;
    }
    size_.fetch_sub(count, std::memory_order_relaxed);

    if (!filePath_.isEmpty()) {
        QMutexLocker lock(&fileMutex_);
        QFile::remove(filePath_);
    }
}

int FileBuffer::size() const {
    return size_.load(std::memory_order_relaxed);
}
