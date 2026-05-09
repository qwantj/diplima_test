#include "monitor_ui/EventHistoryWidget.hpp"
#include "monitor_ui/ThemePalette.hpp"

#include <QHeaderView>
#include <QLabel>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QTextStream>
#include <QLineEdit>
#include <QPainter>
#include <QDateEdit>
#include <QDateTime>

// ====== TimelineWidget ======
TimelineWidget::TimelineWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(60);
    setMaximumHeight(80);
    hourBuckets_.resize(48, 0); // 48 half-hours
}

void TimelineWidget::setEvents(const std::vector<DetectionResult>& events, const QDate& day) {
    day_ = day;
    std::fill(hourBuckets_.begin(), hourBuckets_.end(), 0);

    for (const auto& e : events) {
        if (e.timestamp.date() != day) continue;
        int halfHour = e.timestamp.time().hour() * 2 + (e.timestamp.time().minute() >= 30 ? 1 : 0);
        if (e.label == 1) {
            hourBuckets_[halfHour] = 2; // Attack
        } else if (hourBuckets_[halfHour] == 0) {
            hourBuckets_[halfHour] = 1; // Benign
        }
    }
    update();
}

void TimelineWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int numBlocks = 24; // Draw 24 hourly blocks (can aggregate half-hours or just use 24)
    // Actually let's draw 24 blocks for simplicity, merging the 2 half-hours
    int spacing = 2;
    int blockW = (width() - 40 - (numBlocks - 1) * spacing) / numBlocks;
    if (blockW < 2) blockW = 2;
    int totalW = numBlocks * blockW + (numBlocks - 1) * spacing;
    int startX = (width() - totalW) / 2;
    int startY = 25;
    int blockH = 20;

    for (int i = 0; i < 24; i++) {
        QRect rect(startX + i * (blockW + spacing), startY, blockW, blockH);
        
        int status1 = hourBuckets_[i*2];
        int status2 = hourBuckets_[i*2 + 1];
        int maxStatus = std::max(status1, status2);

        QColor color;
        if (maxStatus == 2) {
            color = ThemePalette::danger();
        } else if (maxStatus == 1) {
            color = ThemePalette::success();
        } else {
            color = QColor("#313244"); // Empty
        }
        
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        p.drawRect(rect);
    }

    // Draw labels: 00:00, 06:00, 12:00, 18:00
    p.setPen(ThemePalette::textSecondary());
    QFont f = font();
    f.setPointSize(9);
    p.setFont(f);
    
    auto drawLabel = [&](int hour, const QString& text) {
        int x = startX + hour * (blockW + spacing);
        QRect r(x - 20, startY + blockH + 2, 40, 20);
        p.drawText(r, Qt::AlignHCenter | Qt::AlignTop, text);
    };
    
    drawLabel(0, "00:00");
    drawLabel(6, "06:00");
    drawLabel(12, "12:00");
    drawLabel(18, "18:00");
}

// ====== EventHistoryWidget ======
EventHistoryWidget::EventHistoryWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    // Top control panel
    auto* controlLayout = new QHBoxLayout();
    
    refreshBtn_ = new QPushButton("Обновить");
    refreshBtn_->setStyleSheet("padding: 4px 12px; background: #313244; color: #cdd6f4; border-radius: 4px;");
    connect(refreshBtn_, &QPushButton::clicked, this, &EventHistoryWidget::refreshData);
    controlLayout->addWidget(refreshBtn_);

    exportBtn_ = new QPushButton("Экспорт в CSV");
    exportBtn_->setStyleSheet("padding: 4px 12px; background: #313244; color: #cdd6f4; border-radius: 4px;");
    connect(exportBtn_, &QPushButton::clicked, this, &EventHistoryWidget::exportToCsv);
    controlLayout->addWidget(exportBtn_);

    controlLayout->addStretch();

    // Filters
    controlLayout->addWidget(new QLabel("Дата:"));
    dateEdit_ = new QDateEdit(QDate::currentDate());
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setStyleSheet("QDateEdit { background: #313244; color: #cdd6f4; padding: 2px; border-radius: 2px; }");
    controlLayout->addWidget(dateEdit_);

    controlLayout->addWidget(new QLabel("Тип:"));
    filterType_ = new QComboBox();
    filterType_->addItems({"Все типы", "Только атаки"});
    filterType_->setStyleSheet("QComboBox { background: #313244; color: #cdd6f4; padding: 2px; border-radius: 2px; }");
    controlLayout->addWidget(filterType_);

    controlLayout->addWidget(new QLabel("IP:"));
    ipFilter_ = new QLineEdit();
    ipFilter_->setPlaceholderText("Фильтр по IP...");
    ipFilter_->setFixedWidth(120);
    ipFilter_->setStyleSheet("QLineEdit { background: #313244; color: #cdd6f4; padding: 2px 4px; border-radius: 2px; }");
    controlLayout->addWidget(ipFilter_);

    layout->addLayout(controlLayout);

    // Uptime Label
    auto* uptimeLabel = new QLabel("Uptime / Временная шкала состояния (выбранный день)");
    uptimeLabel->setStyleSheet("color: #a6adc8; font-size: 11px;");
    layout->addWidget(uptimeLabel);

    // Timeline
    timeline_ = new TimelineWidget(this);
    layout->addWidget(timeline_);

    // Table
    table_ = new QTableWidget(this);
    table_->setColumnCount(6);
    table_->setHorizontalHeaderLabels(
        {"Время начала", "Длительность (с)", "IP атакующего", "Max PPS", "Тип атаки", "Уверенность"});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->verticalHeader()->setVisible(false);
    table_->setAlternatingRowColors(false);
    table_->setEditTriggers(QTableWidget::NoEditTriggers);
    table_->setSelectionBehavior(QTableWidget::SelectRows);
    table_->setShowGrid(false);
    table_->setStyleSheet(R"(
        QTableWidget {
            background: #1e1e2e; color: #cdd6f4;
            border: none; font-size: 12px;
        }
        QTableWidget::item { padding: 4px 8px; border-bottom: 1px solid #313244; }
        QTableWidget::item:selected { background: #313244; }
        QHeaderView::section {
            background: #181825; color: #a6adc8;
            border: none; border-bottom: 1px solid #313244;
            padding: 6px 8px; font-weight: bold; font-size: 12px;
        }
    )");
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

    auto events = dbManager_->getSecurityEvents(1000);

    // Apply filter
    int filterIdx = filterType_->currentIndex();
    QString ipFilterText = ipFilter_->text().trimmed();
    QDate targetDate = dateEdit_->date();

    std::vector<DetectionResult> filtered;
    for (auto& e : events) {
        if (e.timestamp.date() != targetDate) continue;
        if (filterIdx == 1 && e.label != 1) continue;

        QString attackerIP = "Unknown";
        if (!e.topTalkers.empty()) {
            attackerIP = QString::fromStdString(e.topTalkers.front().first);
        }
        if (!ipFilterText.isEmpty() && !attackerIP.contains(ipFilterText)) continue;

        filtered.push_back(e);
    }

    // Update timeline
    timeline_->setEvents(filtered, targetDate);

    // Update table
    table_->setRowCount(0);
    for (auto& e : filtered) {
        int row = table_->rowCount();
        table_->insertRow(row);
        
        table_->setItem(row, 0, new QTableWidgetItem(e.timestamp.toString("yyyy-MM-dd HH:mm:ss")));
        table_->setItem(row, 1, new QTableWidgetItem(QString::number(e.flowDuration, 'f', 1)));
        
        QString attackerIP = "Unknown";
        if (!e.topTalkers.empty()) {
            attackerIP = QString::fromStdString(e.topTalkers.front().first);
        }
        table_->setItem(row, 2, new QTableWidgetItem(attackerIP));
        
        table_->setItem(row, 3, new QTableWidgetItem(QString::number(e.pps, 'f', 0)));
        table_->setItem(row, 4, new QTableWidgetItem(e.label == 1 ? "DDoS" : "Benign"));
        table_->setItem(row, 5, new QTableWidgetItem(QString::number(e.confidence, 'f', 3)));

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
    out << "Время начала,Длительность (с),IP атакующего,Max PPS,Тип атаки,Уверенность\n";
    for (int r = 0; r < table_->rowCount(); r++) {
        for (int c = 0; c < 6; c++) {
            if (c > 0) out << ",";
            out << table_->item(r, c)->text();
        }
        out << "\n";
    }
    f.close();
}
