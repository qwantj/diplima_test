#include "monitor_ui/EventHistoryWidget.hpp"
#include "monitor_ui/ThemePalette.hpp"

#include <QHeaderView>
#include <QLabel>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QTextStream>
#include <QPainterPath>

// ====== TimelineWidget ======
TimelineWidget::TimelineWidget(QWidget* parent) : QWidget(parent) {
    setFixedHeight(70);
    hourBuckets_.resize(48, 0);
}

void TimelineWidget::setEvents(const std::vector<DetectionResult>& events, const QDate& day) {
    day_ = day;
    std::fill(hourBuckets_.begin(), hourBuckets_.end(), 0);
    for (const auto& e : events) {
        if (e.timestamp.date() != day) continue;
        int hour = e.timestamp.time().hour();
        int min = e.timestamp.time().minute();
        int idx = hour * 2 + (min / 30);
        if (idx < 48) {
            if (e.label == 1) hourBuckets_[idx] = 2; // Attack
            else if (hourBuckets_[idx] == 0) hourBuckets_[idx] = 1; // Benign
        }
    }
    update();
}

void TimelineWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    // Draw background
    p.setBrush(QColor(30, 30, 46));
    p.setPen(QPen(QColor(49, 50, 68), 1));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -25), 6, 6);

    int w = (width() - 15) / 48;
    int h = height() - 35;

    for (int i = 0; i < 48; i++) {
        QRect r(8 + i * w, 6, w - 2, h);
        QColor color = QColor(49, 50, 68); // Base / Empty
        if (hourBuckets_[i] == 1) color = ThemePalette::success(); // Benign
        else if (hourBuckets_[i] == 2) color = ThemePalette::danger(); // Attack
        
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(r, 2, 2);
    }

    p.setPen(QColor(166, 173, 200));
    QFont f = p.font(); f.setPointSize(8); f.setBold(true); p.setFont(f);
    p.drawText(8, height() - 5, "00:00");
    p.drawText(width() / 2 - 15, height() - 5, "12:00");
    p.drawText(width() - 45, height() - 5, "23:59");
}

// ====== EventHistoryWidget ======
EventHistoryWidget::EventHistoryWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(15);

    auto* topRow = new QHBoxLayout();
    refreshBtn_ = new QPushButton("Refresh");
    exportBtn_ = new QPushButton("Export to CSV");
    QString btnStyle = R"(
        QPushButton { 
            background: #313244; color: #cdd6f4; border: 1px solid #45475a; 
            border-radius: 4px; padding: 8px 18px; font-weight: bold;
        } 
        QPushButton:hover { background: #45475a; }
    )";
    refreshBtn_->setStyleSheet(btnStyle); exportBtn_->setStyleSheet(btnStyle);
    topRow->addWidget(refreshBtn_); topRow->addWidget(exportBtn_);
    topRow->addStretch();
    
    QLabel* dateLabel = new QLabel("Date:");
    dateLabel->setStyleSheet("color: #a6adc8; font-weight: bold;");
    topRow->addWidget(dateLabel);
    
    dateEdit_ = new QDateEdit(QDate::currentDate());
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setStyleSheet("QDateEdit { background: #313244; color: #cdd6f4; padding: 6px; border-radius: 4px; border: 1px solid #45475a; }");
    topRow->addWidget(dateEdit_);

    filterType_ = new QComboBox();
    filterType_->addItems({"All Types", "DDoS Attack"});
    filterType_->setStyleSheet("QComboBox { background: #313244; color: #cdd6f4; padding: 6px; border-radius: 4px; border: 1px solid #45475a; }");
    topRow->addWidget(filterType_);

    ipFilter_ = new QLineEdit();
    ipFilter_->setPlaceholderText("Filter by IP...");
    ipFilter_->setStyleSheet("QLineEdit { background: #313244; color: #cdd6f4; padding: 6px; border-radius: 4px; border: 1px solid #45475a; }");
    topRow->addWidget(ipFilter_);
    layout->addLayout(topRow);

    timeline_ = new TimelineWidget();
    QLabel* timelineLabel = new QLabel("Uptime / Threat Timeline (24h)");
    timelineLabel->setStyleSheet("color: #cdd6f4; font-weight: bold; text-transform: uppercase; font-size: 11px;");
    layout->addWidget(timelineLabel);
    layout->addWidget(timeline_);

    table_ = new QTableWidget();
    table_->setColumnCount(6);
    table_->setHorizontalHeaderLabels({"Start Time", "Duration (s)", "Attacker IP", "Max PPS", "Type", "Confidence"});
    table_->verticalHeader()->setVisible(false);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setShowGrid(false);
    table_->setAlternatingRowColors(true);
    table_->setStyleSheet(R"(
        QTableWidget {
            background-color: #1e1e2e;
            alternate-background-color: #242437;
            color: #cdd6f4;
            gridline-color: transparent;
            border: 1px solid #313244;
            border-radius: 8px;
        }
        QHeaderView::section {
            background-color: #181825;
            color: #a6adc8;
            font-weight: bold;
            padding: 10px;
            border: none;
            text-transform: uppercase;
            font-size: 11px;
        }
        QTableWidget::item {
            padding: 12px;
            border-bottom: 1px solid #313244;
        }
        QTableWidget::item:selected {
            background-color: #45475a;
            color: #f5e0dc;
        }
    )");
    layout->addWidget(table_);

    connect(refreshBtn_, &QPushButton::clicked, this, &EventHistoryWidget::refreshData);
    connect(exportBtn_, &QPushButton::clicked, this, &EventHistoryWidget::exportToCsv);
    connect(dateEdit_, &QDateEdit::dateChanged, this, &EventHistoryWidget::refreshData);
    connect(filterType_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &EventHistoryWidget::applyFilter);
    connect(ipFilter_, &QLineEdit::textChanged, this, &EventHistoryWidget::applyFilter);
}

void EventHistoryWidget::setDatabaseManager(DatabaseManager* db) {
    dbManager_ = db;
    refreshData();
}

void EventHistoryWidget::refreshData() {
    if (!dbManager_) return;
    auto events = dbManager_->getSecurityEvents(200);
    table_->setRowCount((int)events.size());
    for (int i = 0; i < (int)events.size(); i++) {
        auto& e = events[i];
        table_->setItem(i, 0, new QTableWidgetItem(e.timestamp.toString("yyyy-MM-dd HH:mm:ss")));
        table_->setItem(i, 1, new QTableWidgetItem("10.0")); // dummy duration
        table_->setItem(i, 2, new QTableWidgetItem("unknown"));
        table_->setItem(i, 3, new QTableWidgetItem(QString::number(e.pps, 'f', 1)));
        table_->setItem(i, 4, new QTableWidgetItem("DDoS Attack"));
        table_->setItem(i, 5, new QTableWidgetItem(QString::number(e.confidence, 'f', 2)));
    }
    timeline_->setEvents(events, dateEdit_->date());
    applyFilter();
}

void EventHistoryWidget::applyFilter() {
    QString ip = ipFilter_->text().trimmed();
    int typeIdx = filterType_->currentIndex();

    for (int r = 0; r < table_->rowCount(); r++) {
        bool show = true;
        if (!ip.isEmpty() && !table_->item(r, 2)->text().contains(ip)) show = false;
        if (typeIdx == 1 && table_->item(r, 4)->text() != "DDoS Attack") show = false;
        table_->setRowHidden(r, !show);
    }
}

void EventHistoryWidget::exportToCsv() {
    QString path = QFileDialog::getSaveFileName(this, "Export CSV", "incidents.csv", "CSV files (*.csv)");
    if (path.isEmpty()) return;
    QFile f(path); if(!f.open(QIODevice::WriteOnly)) return;
    QTextStream out(&f);
    out << "StartTime,Duration,AttackerIP,MaxPPS,Type,Confidence\n";
    for (int r = 0; r < table_->rowCount(); r++) {
        if (table_->isRowHidden(r)) continue;
        for (int c = 0; c < 6; c++) {
            if (c > 0) out << ",";
            out << table_->item(r, c)->text();
        }
        out << "\n";
    }
    f.close();
}
