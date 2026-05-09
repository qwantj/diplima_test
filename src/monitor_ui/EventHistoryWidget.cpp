#include "monitor_ui/EventHistoryWidget.hpp"
#include "monitor_ui/ThemePalette.hpp"

#include <QHeaderView>
#include <QLabel>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QTextStream>

// ====== TimelineWidget ======
TimelineWidget::TimelineWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(60);
    setMaximumHeight(80);
}

void TimelineWidget::setEvents(const std::vector<DetectionResult>& events) {
    events_ = events;
    update();
}

void TimelineWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), ThemePalette::cardBackground());

    if (events_.empty()) {
        p.setPen(ThemePalette::textSecondary());
        p.drawText(rect(), Qt::AlignCenter, "No security events");
        return;
    }

    // Find time range
    qint64 minT = events_.front().timestamp.toMSecsSinceEpoch();
    qint64 maxT = events_.back().timestamp.toMSecsSinceEpoch();
    qint64 range = std::max(maxT - minT, (qint64)1);

    int w = width() - 20;
    int y = height() / 2;

    // Draw timeline line
    p.setPen(QPen(ThemePalette::textSecondary(), 1));
    p.drawLine(10, y, 10 + w, y);

    // Draw events
    for (auto& e : events_) {
        qint64 t = e.timestamp.toMSecsSinceEpoch();
        int x = 10 + (int)((double)(t - minT) / range * w);
        QColor c = e.label == 1 ? ThemePalette::danger() : ThemePalette::success();
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPoint(x, y), 4, 4);
    }
}

// ====== EventHistoryWidget ======
EventHistoryWidget::EventHistoryWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);

    auto* title = new QLabel("Security Incidents");
    title->setStyleSheet("font-size: 14px; font-weight: bold; padding: 4px;");
    layout->addWidget(title);

    // Timeline
    timeline_ = new TimelineWidget(this);
    layout->addWidget(timeline_);

    // Filters
    auto* filterLayout = new QHBoxLayout();
    filterType_ = new QComboBox();
    filterType_->addItems({"All", "Attacks Only", "High Confidence (>0.9)"});
    filterLayout->addWidget(new QLabel("Type:"));
    filterLayout->addWidget(filterType_);

    dateFrom_ = new QDateEdit(QDate::currentDate().addDays(-7));
    dateTo_ = new QDateEdit(QDate::currentDate());
    filterLayout->addWidget(new QLabel("From:"));
    filterLayout->addWidget(dateFrom_);
    filterLayout->addWidget(new QLabel("To:"));
    filterLayout->addWidget(dateTo_);

    refreshBtn_ = new QPushButton("Refresh");
    connect(refreshBtn_, &QPushButton::clicked, this, &EventHistoryWidget::refreshData);
    filterLayout->addWidget(refreshBtn_);

    exportBtn_ = new QPushButton("Export CSV");
    connect(exportBtn_, &QPushButton::clicked, this, &EventHistoryWidget::exportToCsv);
    filterLayout->addWidget(exportBtn_);

    filterLayout->addStretch();
    layout->addLayout(filterLayout);

    // Table
    table_ = new QTableWidget(this);
    table_->setColumnCount(6);
    table_->setHorizontalHeaderLabels(
        {"Time", "Label", "Confidence", "PPS", "Duration", "Model"});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->verticalHeader()->setVisible(false);
    table_->setAlternatingRowColors(true);
    table_->setEditTriggers(QTableWidget::NoEditTriggers);
    table_->setSelectionBehavior(QTableWidget::SelectRows);
    layout->addWidget(table_);
}

void EventHistoryWidget::setDatabaseManager(DatabaseManager* dbManager) {
    dbManager_ = dbManager;
}

void EventHistoryWidget::refreshData() {
    applyFilter();
}

void EventHistoryWidget::applyFilter() {
    if (!dbManager_) return;

    auto events = dbManager_->getSecurityEvents(500);

    // Apply filter
    int filterIdx = filterType_->currentIndex();
    std::vector<DetectionResult> filtered;
    for (auto& e : events) {
        if (filterIdx == 1 && e.label != 1) continue;
        if (filterIdx == 2 && e.confidence < 0.9f) continue;
        filtered.push_back(e);
    }

    // Update timeline
    timeline_->setEvents(filtered);

    // Update table
    table_->setRowCount(0);
    for (auto& e : filtered) {
        int row = table_->rowCount();
        table_->insertRow(row);
        table_->setItem(row, 0, new QTableWidgetItem(e.timestamp.toString("yyyy-MM-dd HH:mm:ss")));
        table_->setItem(row, 1, new QTableWidgetItem(e.label == 1 ? "ATTACK" : "Benign"));
        table_->setItem(row, 2, new QTableWidgetItem(QString::number(e.confidence, 'f', 3)));
        table_->setItem(row, 3, new QTableWidgetItem(QString::number(e.pps, 'f', 0)));
        table_->setItem(row, 4, new QTableWidgetItem(QString::number(e.flowDuration, 'f', 1) + "s"));
        table_->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(e.modelName)));

        if (e.label == 1) {
            for (int c = 0; c < 6; c++)
                table_->item(row, c)->setForeground(ThemePalette::danger());
        }
    }
}

void EventHistoryWidget::exportToCsv() {
    QString path = QFileDialog::getSaveFileName(this, "Export to CSV", "", "CSV (*.csv)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&f);
    out << "Time,Label,Confidence,PPS,Duration,Model\n";
    for (int r = 0; r < table_->rowCount(); r++) {
        for (int c = 0; c < 6; c++) {
            if (c > 0) out << ",";
            out << table_->item(r, c)->text();
        }
        out << "\n";
    }
    f.close();
}
