#pragma once

#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QByteArray>
#include <vector>
#include <nlohmann/json.hpp>
#include "common/FileBuffer.hpp"

class TcpServer : public QTcpServer {
    Q_OBJECT
public:
    explicit TcpServer(QObject* parent = nullptr);
    ~TcpServer() override;

    bool startListening(const QString& address = "127.0.0.1", quint16 port = 50050);
    void broadcast(const QByteArray& data);
    int  clientCount() const;

protected:
    void incomingConnection(qintptr socketDescriptor) override;

signals:
    void commandReceived(const std::string& cmd, const nlohmann::json& data);

private slots:
    void onClientDisconnected();
    void onReadyRead();

private:
    QList<QTcpSocket*> clients_;
    FileBuffer fileBuffer_;
};
