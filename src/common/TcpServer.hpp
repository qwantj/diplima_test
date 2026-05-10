#pragma once

#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QByteArray>
#include <vector>
#include <deque>
#include <nlohmann/json.hpp>

class TcpServer : public QTcpServer {
    Q_OBJECT
public:
    explicit TcpServer(QObject* parent = nullptr);
    ~TcpServer() override;

    bool startListening(quint16 port = 50050);
    void broadcast(const QByteArray& data);
    int  clientCount() const;

    void setBufferSize(int maxItems);
    std::vector<QByteArray> getRecentBuffer() const;

protected:
    void incomingConnection(qintptr socketDescriptor) override;

signals:
    void commandReceived(const std::string& cmd, const nlohmann::json& data);

private slots:
    void onClientDisconnected();
    void onReadyRead();

private:
    QList<QTcpSocket*> clients_;
    std::deque<QByteArray> recentBuffer_;
    int maxBufferSize_ = 500;
};
