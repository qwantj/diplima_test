/**
 * @file TcpServer.hpp
 * @brief Модуль сетевого сервера (заголовок).
 *
 * Назначение: Обеспечение связи коллектора с мониторами и трансляция данных.
 * Входные данные: Параметры прослушивания (IP, порт).
 * Результаты: Рассылка DetectionResult подключенным клиентам.
 * Метод решения: Использование QTcpServer для управления списком активных подключений.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

#pragma once // Директива защиты от повторного включения

#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QByteArray>
#include <vector>
#include <nlohmann/json.hpp>
#include "common/FileBuffer.hpp"

/**
 * @class TcpServer
 * @brief Серверный компонент для передачи телеметрии по сети.
 */
class TcpServer : public QTcpServer {
  Q_OBJECT
public:
  explicit TcpServer(QObject* parent = nullptr);
  ~TcpServer() override;

  // Запуск прослушивания порта
  bool startListening(const QString& address = "127.0.0.1", quint16 port = 50050);
  
  // Рассылка сообщения всем клиентам
  void broadcast(const QByteArray& data);
  
  // Количество активных клиентов
  int  clientCount() const;

protected:
  // Обработчик нового подключения
  void incomingConnection(qintptr socketDescriptor) override;

signals:
  // Сигнал получения команды от клиента
  void commandReceived(const std::string& cmd, const nlohmann::json& data);

private slots:
  void onClientDisconnected();
  void onReadyRead();

private:
  QList<QTcpSocket*> clients_; // Список подключенных клиентов
  FileBuffer fileBuffer_;      // Буфер для временного хранения при отсутствии связи
};
