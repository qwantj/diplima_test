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
    enum class BucketType { Empty = 0, Benign = 1, Attack = 2 };
    static constexpr int NUM_BUCKETS = 48;

    explicit TimelineWidget(QWidget* parent = nullptr);
    void setEvents(const std::vector<DetectionResult>& events, const std::vector<SessionInfo>& sessions, const QDate& day);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    std::vector<BucketType> hourBuckets_; // 48 half-hour buckets
    QDate day_;
};

class EventHistoryWidget : public QWidget {
    Q_OBJECT
public:
    explicit EventHistoryWidget(QWidget* parent = nullptr);
    void setDatabaseManager(DatabaseManager* dbManager);

protected:
    void showEvent(QShowEvent* event) override;

private slots:
    void refreshData();
    void exportToCsv();
    void exportToPdf();
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
    QPushButton* exportPdfBtn_ = nullptr;
};
