#pragma once

#include <QByteArray>
#include <QFile>
#include <QMutex>
#include <QString>
#include <deque>

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
    std::deque<QByteArray> buffer_;
    mutable QMutex mutex_;
};
