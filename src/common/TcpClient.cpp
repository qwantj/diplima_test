#include "common/TcpClient.hpp"
#include "common/AppLogger.hpp"
#include <QJsonDocument>
#include <nlohmann/json.hpp>

TcpClient::TcpClient(QObject* parent)
    : QObject(parent)
    , socket_(new QTcpSocket(this))
    , reconnectTimer_(new QTimer(this))
{
    reconnectTimer_->setInterval(5000);
    connect(socket_, &QTcpSocket::connected, this, &TcpClient::onConnected);
    connect(socket_, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
    connect(socket_, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(reconnectTimer_, &QTimer::timeout, this, &TcpClient::tryReconnect);
}

TcpClient::~TcpClient() {
    reconnectTimer_->stop();
    disconnectFromCollector();
}

void TcpClient::connectToCollector(const QString& host, quint16 port) {
    host_ = host;
    port_ = port;
    socket_->connectToHost(host, port);
}

void TcpClient::disconnectFromCollector() {
    reconnectTimer_->stop();
    if (socket_->state() != QTcpSocket::UnconnectedState)
        socket_->disconnectFromHost();
}

bool TcpClient::isConnected() const {
    return socket_->state() == QTcpSocket::ConnectedState;
}

void TcpClient::sendCommand(const std::string& cmd, const nlohmann::json& data) {
    if (!isConnected()) return;
    socket_->write(Protocol::serializeCommand(cmd, data));
    socket_->flush();
}

void TcpClient::onConnected() {
    reconnectTimer_->stop();
    AppLogger::get()->info("Connected to collector.");
    emit connectedToServer();
}

void TcpClient::onDisconnected() {
    AppLogger::get()->warn("Disconnected from collector. Will retry...");
    emit disconnectedFromServer();
    reconnectTimer_->start();
}

void TcpClient::onReadyRead() {
    readBuffer_.append(socket_->readAll());

    while (true) {
        int idx = readBuffer_.indexOf('\n');
        if (idx < 0) break;
        QByteArray line = readBuffer_.left(idx).trimmed();
        readBuffer_.remove(0, idx + 1);
        if (!line.isEmpty())
            processLine(line);
    }
}

void TcpClient::tryReconnect() {
    if (socket_->state() == QTcpSocket::UnconnectedState)
        socket_->connectToHost(host_, port_);
}

void TcpClient::processLine(const QByteArray& line) {
    auto [type, payload] = Protocol::parseMessage(line);

    if (type == Protocol::MSG_STATS) {
        auto result = Protocol::deserializeResult(payload["data"]);
        uint64_t total = payload.value("total_packets", (uint64_t)0);
        emit statsReceived(result, total);
    }
    else if (type == Protocol::MSG_SNAPSHOT) {
        float pps = payload.value("pps", 0.0f);
        uint64_t total = payload.value("total_packets", (uint64_t)0);
        int label = payload.value("current_label", 0);
        emit snapshotReceived(pps, total, label);
    }
    else if (type == Protocol::MSG_BUFFER) {
        std::vector<QByteArray> items;
        if (payload.contains("items")) {
            for (auto& item : payload["items"])
                items.push_back(QByteArray::fromStdString(item.dump()));
        }
        AppLogger::get()->info("Received buffer with {} items.", items.size());
        emit bufferReceived(items);
    }
    else if (type == Protocol::MSG_NOTIFY) {
        QString event = QString::fromStdString(payload.value("event", ""));
        QJsonObject data;
        if (payload.contains("data")) {
            auto doc = QJsonDocument::fromJson(
                QByteArray::fromStdString(payload["data"].dump()));
            data = doc.object();
        }
        emit notificationReceived(event, data);
    }
}
