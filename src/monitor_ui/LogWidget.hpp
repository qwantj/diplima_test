#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QVBoxLayout>
#include <deque>

#include "common/Protocol.hpp"

class LogWidget : public QWidget {
    Q_OBJECT
public:
    explicit LogWidget(QWidget* parent = nullptr);

    void addEvent(const DetectionResult& r);
    void loadHistory(const std::vector<DetectionResult>& events);

private:
    void appendRow(const DetectionResult& r);

    QTableWidget* table_ = nullptr;
    QComboBox*    filterCombo_ = nullptr;
    std::deque<DetectionResult> buffer_;
    static constexpr int MAX_ROWS = 2000;
};
