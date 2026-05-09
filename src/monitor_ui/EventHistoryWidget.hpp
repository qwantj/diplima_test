#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QDateEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QPainter>

#include "common/DatabaseManager.hpp"
#include "common/Protocol.hpp"

// 24-hour timeline with colored squares
class TimelineWidget : public QWidget {
    Q_OBJECT
public:
    explicit TimelineWidget(QWidget* parent = nullptr);
    void setEvents(const std::vector<DetectionResult>& events, const QDate& day);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    std::vector<int> hourBuckets_; // 48 half-hour buckets: 0=empty, 1=benign, 2=attack
    QDate day_;
};

class EventHistoryWidget : public QWidget {
    Q_OBJECT
public:
    explicit EventHistoryWidget(QWidget* parent = nullptr);
    void setDatabaseManager(DatabaseManager* dbManager);

private slots:
    void refreshData();
    void exportToCsv();
    void applyFilter();

private:
    DatabaseManager* dbManager_ = nullptr;
    TimelineWidget* timeline_ = nullptr;
    QTableWidget* table_ = nullptr;
    QComboBox* filterType_ = nullptr;
    QDateEdit* dateEdit_ = nullptr;
    QLineEdit* ipFilter_ = nullptr;
    QPushButton* refreshBtn_ = nullptr;
    QPushButton* exportBtn_ = nullptr;
};
