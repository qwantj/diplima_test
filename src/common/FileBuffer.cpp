#include "common/FileBuffer.hpp"

FileBuffer::FileBuffer(const QString& filePath) : filePath_(filePath) {}

void FileBuffer::setFilePath(const QString& path) { filePath_ = path; }

void FileBuffer::append(const QByteArray& data) {
    QMutexLocker lock(&mutex_);
    buffer_.push_back(data);
    if (!filePath_.isEmpty()) {
        QFile f(filePath_);
        if (f.open(QIODevice::Append)) {
            f.write(data);
            f.write("\n");
            f.close();
        }
    }
}

std::deque<QByteArray> FileBuffer::readAll() {
    QMutexLocker lock(&mutex_);
    return buffer_;
}

void FileBuffer::clear() {
    QMutexLocker lock(&mutex_);
    buffer_.clear();
    if (!filePath_.isEmpty()) {
        QFile::remove(filePath_);
    }
}

int FileBuffer::size() const {
    QMutexLocker lock(&mutex_);
    return (int)buffer_.size();
}
