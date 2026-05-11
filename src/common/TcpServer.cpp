#include "common/TcpServer.hpp"
#include "common/AppLogger.hpp"
#include "common/Protocol.hpp"
#include <nlohmann/json.hpp>

TcpServer::TcpServer(QObject* parent) : QTcpServer(parent), fileBuffer_("tcp_buffer.jsonl") {}

TcpServer::~TcpServer() {
    for (auto* c : clients_) {
        c->disconnectFromHost();
        c->deleteLater();
    }
    clients_.clear();
}

bool TcpServer::startListening(const QString& address, quint16 port) {
    if (!listen(QHostAddress(address), port)) {
        AppLogger::get()->error("TcpServer: failed to listen on {} port {}: {}",
            address.toStdString(), port, errorString().toStdString());
        return false;
    }
    AppLogger::get()->info("TcpServer: listening on {} port {}.", address.toStdString(), port);
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

    // If we have buffered offline messages, send them to the first connected client
    if (fileBuffer_.size() > 0) {
        auto messages = fileBuffer_.readAllAndClear();
        nlohmann::json bufferMsg;
        bufferMsg["type"] = Protocol::MSG_BUFFER;
        nlohmann::json items = nlohmann::json::array();
        for (const auto& msg : messages) {
            try {
                items.push_back(nlohmann::json::parse(msg.toStdString()));
            } catch (const std::exception& e) {
                std::string rawData = msg.toStdString();
                if (rawData.length() > 100) {
                    rawData = rawData.substr(0, 100) + "...";
                }
                AppLogger::get()->warn("TcpServer: failed to parse buffered message: {}. Raw data: {}", e.what(), rawData);
            }
        }
        bufferMsg["items"] = items;
        QByteArray framed = Protocol::frame(QByteArray::fromStdString(bufferMsg.dump()));
        socket->write(framed);
        socket->flush();
        AppLogger::get()->info("TcpServer: sent {} buffered offline messages to new client.", messages.size());
    }
}

void TcpServer::broadcast(const QByteArray& data) {
    if (clients_.isEmpty()) {
        // Buffer to disk if no clients are connected
        fileBuffer_.push(data);
    } else {
        // Send to all connected clients
        for (int i = clients_.size() - 1; i >= 0; --i) {
            auto* c = clients_[i];
            if (c->state() == QTcpSocket::ConnectedState) {
                c->write(data);
                c->flush();
            }
        }
    }
}

int TcpServer::clientCount() const { return clients_.size(); }

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
            std::string cmd = payload.value("cmd", "");
            nlohmann::json data = payload.value("data", nlohmann::json::object());
            
            AppLogger::get()->info("TcpServer: received command: {}", cmd);
            emit commandReceived(cmd, data);
        }
    }
}
