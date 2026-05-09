#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <vector>

#include "common/Protocol.hpp"

class SessionWidget : public QWidget {
    Q_OBJECT
public:
    explicit SessionWidget(QWidget* parent = nullptr);

    void loadSessions(const std::vector<SessionInfo>& sessions);

signals:
    void sessionSelected(int sessionId);

private:
    QTableWidget* table_ = nullptr;
};
