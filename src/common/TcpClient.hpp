/**
 * @file TcpClient.hpp
 * @brief Модуль сетевого клиента (заголовок).
 *
 * Назначение: Подключение к коллектору и получение оперативных данных мониторинга.
 * Входные данные: IP и порт коллектора.
 * Результаты: Сигналы с результатами анализа для GUI.
 * Метод решения: Использование QTcpSocket для асинхронного взаимодействия.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

#pragma once // Директива защиты от повторного включения

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QByteArray>
#include <QJsonObject>
#include <vector>

#include "common/Protocol.hpp"

/**
 * @class TcpClient
 * @brief Класс для связи монитора с сервисом сбора данных.
 */
class TcpClient : public QObject {
  Q_OBJECT
public:
  explicit TcpClient(QObject* parent = nullptr);
  ~TcpClient() override;

  // Установка соединения
  void connectToCollector(const QString& host = "localhost", quint16 port = 50050);
  void disconnectFromCollector();
  bool isConnected() const;

  // Отправка команды коллектору
  void sendCommand(const std::string& cmd, const nlohmann::json& data = {});

signals:
  void connectedToServer();
  void disconnectedFromServer();
  void statsReceived(const DetectionResult& result, uint64_t totalPackets);
  void snapshotReceived(float pps, uint64_t totalPackets, int currentLabel);
  void bufferReceived(const std::vector<QByteArray>& rawMessages);
  void notificationReceived(const QString& event, const QJsonObject& data);

private slots:
  void onConnected();
  void onDisconnected();
  void onReadyRead();
  void tryReconnect();

private:
  void processLine(const QByteArray& line);

  QTcpSocket* socket_ = nullptr;
  QTimer* reconnectTimer_ = nullptr;
  QString host_;
  quint16 port_ = 50050;
  QByteArray readBuffer_; // Буфер для сборки пакетов
};
