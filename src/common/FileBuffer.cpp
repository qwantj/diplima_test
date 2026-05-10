#include "common/FileBuffer.hpp"
#include "common/AppLogger.hpp"
#include <QFileInfo>
#include <QTextStream>

FileBuffer::FileBuffer(const QString& filePath, size_t maxMemoryItems)
    : filePath_(filePath), maxMemoryItems_(maxMemoryItems) {}

FileBuffer::~FileBuffer() {
    flushToDisk();
}

void FileBuffer::setFilePath(const QString& path) {
    QMutexLocker lock(&mutex_);
    filePath_ = path;
}

void FileBuffer::push(const QByteArray& data) {
    QMutexLocker lock(&mutex_);
    memoryBuffer_.push_back(data);
    totalSize_.fetch_add(1, std::memory_order_relaxed);

    if (memoryBuffer_.size() > maxMemoryItems_) {
        flushToDisk();
    }
}

void FileBuffer::flushToDisk() {
    if (filePath_.isEmpty() || memoryBuffer_.empty()) return;

    QFile f(filePath_);
    if (f.open(QIODevice::Append | QIODevice::WriteOnly)) {
        for (const auto& item : memoryBuffer_) {
            f.write(item);
            if (!item.endsWith('\n')) {
                f.write("\n");
            }
        }
        f.close();
        memoryBuffer_.clear();
    } else {
        AppLogger::get()->error("FileBuffer: Failed to open file for writing: {}", filePath_.toStdString());
        // If file opening fails, we have to keep elements in memory, which might lead to OOM.
        // We could cap the memory buffer to prevent an absolute crash.
        while (memoryBuffer_.size() > maxMemoryItems_ * 2) {
            memoryBuffer_.pop_front();
            totalSize_.fetch_sub(1, std::memory_order_relaxed);
        }
    }
}

std::vector<QByteArray> FileBuffer::readAllAndClear() {
    QMutexLocker lock(&mutex_);
    std::vector<QByteArray> result;

    // Read from disk first to maintain chronological order
    if (!filePath_.isEmpty() && QFileInfo::exists(filePath_)) {
        QFile f(filePath_);
        if (f.open(QIODevice::ReadOnly)) {
            while (!f.atEnd()) {
                QByteArray line = f.readLine();
                if (line.endsWith('\n')) line.chop(1);
                if (!line.isEmpty()) {
                    result.push_back(line);
                }
            }
            f.close();
            f.remove();
        }
    }

    // Append memory buffer items
    for (const auto& item : memoryBuffer_) {
        QByteArray data = item;
        if (data.endsWith('\n')) data.chop(1);
        result.push_back(data);
    }
    memoryBuffer_.clear();
    totalSize_.store(0, std::memory_order_relaxed);

    return result;
}

void FileBuffer::clear() {
    QMutexLocker lock(&mutex_);
    memoryBuffer_.clear();
    totalSize_.store(0, std::memory_order_relaxed);
    if (!filePath_.isEmpty() && QFileInfo::exists(filePath_)) {
        QFile::remove(filePath_);
    }
}

int FileBuffer::size() const {
    return totalSize_.load(std::memory_order_relaxed);
}
