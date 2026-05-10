#include "monitor_ui/EventHistoryWidget.hpp"
#include "monitor_ui/ThemePalette.hpp"

#include <QHeaderView>
#include <QLabel>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QTextStream>

// ====== TimelineWidget (24h squares) ======
TimelineWidget::TimelineWidget(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(50);
    setMaximumHeight(70);
    hourBuckets_.resize(48, 0);
    day_ = QDate::currentDate();
}

void TimelineWidget::setEvents(const std::vector<DetectionResult>& events, const QDate& day) {
    day_ = day;
    hourBuckets_.assign(48, 0);
    for (auto& e : events) {
        if (e.timestamp.date() != day) continue;
        int halfHour = e.timestamp.time().hour() * 2 + (e.timestamp.time().minute() >= 30 ? 1 : 0);
        if (halfHour >= 0 && halfHour < 48) {
            if (e.label == 1)
                hourBuckets_[halfHour] = 2; // attack
            else if (hourBuckets_[halfHour] == 0)
                hourBuckets_[halfHour] = 1; // benign activity
        }
    }
    update();
}

void TimelineWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#1e1e2e"));

    int margin = 10;
    int availW = width() - 2 * margin;
    int cellW = availW / 48;
    int cellH = 20;
    int y0 = 10;

    for (int i = 0; i < 48; i++) {
        QRect r(margin + i * cellW, y0, cellW - 2, cellH);
        QColor c;
        if (hourBuckets_[i] == 2)
            c = QColor("#a6e3a1"); // attack = bright green (matching screenshot)
        else if (hourBuckets_[i] == 1)
            c = QColor("#45475a"); // activity = dim
        else
            c = QColor("#313244"); // empty = dark
        p.fillRect(r, c);
    }

    // Time labels
    p.setPen(QColor("#6c7086"));
    p.setFont(QFont("Segoe UI", 8));
    int labelY = y0 + cellH + 14;
    for (int h = 0; h <= 24; h += 6) {
        int x = margin + h * 2 * cellW;
        p.drawText(x - 15, labelY, QString("%1:00").arg(h, 2, 10, QChar('0')));
    }
}

// ====== EventHistoryWidget ======
EventHistoryWidget::EventHistoryWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);

    // Top bar: buttons + filters
    auto* topBar = new QHBoxLayout();

    refreshBtn_ = new QPushButton(QString::fromUtf8("Обновить"));
    refreshBtn_->setStyleSheet("QPushButton { background: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 4px; padding: 5px 12px; } QPushButton:hover { background: #45475a; }");
    connect(refreshBtn_, &QPushButton::clicked, this, &EventHistoryWidget::refreshData);
    topBar->addWidget(refreshBtn_);

    exportBtn_ = new QPushButton(QString::fromUtf8("Экспорт в CSV"));
    exportBtn_->setStyleSheet("QPushButton { background: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 4px; padding: 5px 12px; } QPushButton:hover { background: #45475a; }");
    connect(exportBtn_, &QPushButton::clicked, this, &EventHistoryWidget::exportToCsv);
    topBar->addWidget(exportBtn_);

    topBar->addStretch();

    topBar->addWidget(new QLabel(QString::fromUtf8("Дата:")));
    dateEdit_ = new QDateEdit(QDate::currentDate());
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setStyleSheet("QDateEdit { background: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 4px; padding: 4px; }");
    topBar->addWidget(dateEdit_);

    topBar->addWidget(new QLabel(QString::fromUtf8("Тип:")));
    filterType_ = new QComboBox();
    filterType_->addItems({QString::fromUtf8("Все типы"), "UDP Flood", "TCP SYN Flood", "ICMP Flood"});
    filterType_->setStyleSheet("QComboBox { background: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 4px; padding: 4px 8px; }");
    topBar->addWidget(filterType_);

    topBar->addWidget(new QLabel("IP:"));
    ipFilter_ = new QLineEdit();
    ipFilter_->setPlaceholderText(QString::fromUtf8("Фильтр по IP..."));
    ipFilter_->setFixedWidth(140);
    ipFilter_->setStyleSheet("QLineEdit { background: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 4px; padding: 4px 8px; }");
    topBar->addWidget(ipFilter_);

    layout->addLayout(topBar);

    // Timeline title
    auto* tlLabel = new QLabel(QString::fromUtf8("Uptime / Временная шкала состояния (выбранный день)"));
    tlLabel->setStyleSheet("color: #a6adc8; font-size: 11px; padding: 2px;");
    layout->addWidget(tlLabel);

    // Timeline
    timeline_ = new TimelineWidget(this);
    layout->addWidget(timeline_);

    // Table
    table_ = new QTableWidget(this);
    table_->setColumnCount(6);
    table_->setHorizontalHeaderLabels({
        QString::fromUtf8("Время начала"),
        QString::fromUtf8("Длительность (с)"),
        QString::fromUtf8("IP атакующего"),
        "Max PPS",
        QString::fromUtf8("Тип атаки"),
        QString::fromUtf8("Уверенность")
    });
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_->verticalHeader()->setVisible(false);
    table_->setEditTriggers(QTableWidget::NoEditTriggers);
    table_->setSelectionBehavior(QTableWidget::SelectRows);
    table_->setShowGrid(false);
    table_->setStyleSheet(R"(
        QTableWidget {
            background: #1e1e2e; color: #cdd6f4;
            border: none; font-size: 12px;
        }
        QTableWidget::item { padding: 4px 8px; border-bottom: 1px solid #252535; }
        QTableWidget::item:selected { background: #313244; }
        QHeaderView::section {
            background: #181825; color: #a6adc8;
            border: none; border-bottom: 1px solid #313244;
            padding: 6px 8px; font-weight: bold; font-size: 12px;
        }
    )");
    layout->addWidget(table_);

    connect(dateEdit_, &QDateEdit::dateChanged, [this](const QDate&) { applyFilter(); });
    connect(filterType_, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int) { applyFilter(); });
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
    QDate selectedDate = dateEdit_->date();
    QString ipFilt = ipFilter_->text().trimmed();

    // Filter by date and aggregate consecutive attacks into incidents
    std::vector<DetectionResult> dayEvents;
    for (auto& e : events) {
        if (e.timestamp.date() != selectedDate) continue;
        if (e.label != 1) continue; // only attacks
        if (!ipFilt.isEmpty()) {
            // Check if any top talker matches
            bool match = false;
            for (auto& t : e.topTalkers) {
                if (QString::fromStdString(t.first).contains(ipFilt))
                    match = true;
            }
            if (!match) continue;
        }
        dayEvents.push_back(e);
    }

    // Update timeline (pass all events for the day including benign)
    std::vector<DetectionResult> allDayEvents;
    for (auto& e : events) {
        if (e.timestamp.date() == selectedDate)
            allDayEvents.push_back(e);
    }
    timeline_->setEvents(allDayEvents, selectedDate);

    // Merge consecutive attack windows into incidents
    table_->setRowCount(0);
    if (dayEvents.empty()) return;

    // Each attack event = one row for now (could merge later)
    for (auto& e : dayEvents) {
        int row = table_->rowCount();
        table_->insertRow(row);

        table_->setItem(row, 0, new QTableWidgetItem(e.timestamp.toString("HH:mm:ss")));
        table_->setItem(row, 1, new QTableWidgetItem(QString::number(e.flowDuration, 'f', 1)));

        // Top attacker IP
        QString attackerIp = "-";
        if (!e.topTalkers.empty())
            attackerIp = QString::fromStdString(e.topTalkers[0].first);
        table_->setItem(row, 2, new QTableWidgetItem(attackerIp));

        table_->setItem(row, 3, new QTableWidgetItem(QString::number(e.pps, 'f', 0)));

        // Attack type heuristic
        QString attackType = "Mixed";
        if (e.udpPackets > e.tcpPackets && e.udpPackets > e.icmpPackets)
            attackType = "UDP Flood";
        else if (e.tcpPackets > e.udpPackets && e.synPackets > e.tcpPackets / 2)
            attackType = "TCP SYN Flood";
        else if (e.icmpPackets > e.tcpPackets && e.icmpPackets > e.udpPackets)
            attackType = "ICMP Flood";
        table_->setItem(row, 4, new QTableWidgetItem(attackType));

        table_->setItem(row, 5, new QTableWidgetItem(QString::number(e.confidence, 'f', 2)));

        // Color attack rows
        for (int c = 0; c < 6; c++)
            table_->item(row, c)->setForeground(QColor("#f38ba8"));
    }
}

void EventHistoryWidget::exportToCsv() {
    QString path = QFileDialog::getSaveFileName(this, "Export to CSV", "", "CSV (*.csv)");
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&f);
    out << "Start Time,Duration(s),Attacker IP,Max PPS,Attack Type,Confidence\n";
    for (int r = 0; r < table_->rowCount(); r++) {
        for (int c = 0; c < 6; c++) {
            if (c > 0) out << ",";
            out << table_->item(r, c)->text();
        }
        out << "\n";
    }
    f.close();
}
