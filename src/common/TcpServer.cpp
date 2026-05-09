#include "common/TcpServer.hpp"
#include "common/AppLogger.hpp"
#include "common/Protocol.hpp"
#include <nlohmann/json.hpp>

TcpServer::TcpServer(QObject* parent) : QTcpServer(parent) {}

TcpServer::~TcpServer() {
    for (auto* c : clients_) {
        c->disconnectFromHost();
        c->deleteLater();
    }
    clients_.clear();
}

bool TcpServer::startListening(quint16 port) {
    if (!listen(QHostAddress::Any, port)) {
        AppLogger::get()->error("TcpServer: failed to listen on port {}: {}",
            port, errorString().toStdString());
        return false;
    }
    AppLogger::get()->info("TcpServer: listening on port {}.", port);
    return true;
}

void TcpServer::incomingConnection(qintptr socketDescriptor) {
    auto* socket = new QTcpSocket(this);
    if (!socket->setSocketDescriptor(socketDescriptor)) {
        delete socket;
        return;
    }

    clients_.append(socket);
    connect(socket, &QTcpSocket::disconnected, this, &TcpServer::onClientDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &TcpServer::onReadyRead);

    AppLogger::get()->info("TcpServer: client connected from {}:{}. Total clients: {}",
        socket->peerAddress().toString().toStdString(),
        socket->peerPort(), clients_.size());

    // Send buffer of recent messages
    nlohmann::json bufferMsg;
    bufferMsg["type"] = Protocol::MSG_BUFFER;
    nlohmann::json items = nlohmann::json::array();
    for (const auto& msg : recentBuffer_)
        items.push_back(nlohmann::json::parse(msg.toStdString()));
    bufferMsg["items"] = items;
    QByteArray framed = Protocol::frame(QByteArray::fromStdString(bufferMsg.dump()));
    socket->write(framed);
    socket->flush();
}

void TcpServer::broadcast(const QByteArray& data) {
    // Store in recent buffer
    recentBuffer_.push_back(data);
    while ((int)recentBuffer_.size() > maxBufferSize_)
        recentBuffer_.pop_front();

    // Send to all connected clients
    for (int i = clients_.size() - 1; i >= 0; --i) {
        auto* c = clients_[i];
        if (c->state() == QTcpSocket::ConnectedState) {
            c->write(data);
            c->flush();
        }
    }
}

int TcpServer::clientCount() const { return clients_.size(); }

void TcpServer::setBufferSize(int maxItems) { maxBufferSize_ = maxItems; }

std::vector<QByteArray> TcpServer::getRecentBuffer() const {
    return {recentBuffer_.begin(), recentBuffer_.end()};
}

void TcpServer::onClientDisconnected() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        clients_.removeAll(socket);
        socket->deleteLater();
        AppLogger::get()->info("TcpServer: client disconnected. Remaining: {}", clients_.size());
    }
}

void TcpServer::onReadyRead() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    while (socket->canReadLine()) {
        QByteArray line = socket->readLine().trimmed();
        if (line.isEmpty()) continue;

        auto [type, payload] = Protocol::parseMessage(line);
        if (type == Protocol::MSG_CMD) {
            // Forward commands to be handled by collector
            AppLogger::get()->info("TcpServer: received command: {}",
                payload.value("cmd", "unknown"));
            emit newConnection(); // reuse signal as "command received"
        }
    }
}
