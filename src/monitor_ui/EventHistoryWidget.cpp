#include "monitor_ui/EventHistoryWidget.hpp"
#include "monitor_ui/ThemePalette.hpp"
#include "common/CSVUtils.hpp"
#include "monitor_ui/ReportGenerator.hpp"

#include <QHeaderView>
#include <QLabel>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QTextStream>
#include <QPainterPath>
#include <QMessageBox>

// ====== TimelineWidget ======
TimelineWidget::TimelineWidget(QWidget* parent) : QWidget(parent) {
    setFixedHeight(70);
    hourBuckets_.resize(NUM_BUCKETS, BucketType::Empty);
}

void TimelineWidget::setEvents(const std::vector<DetectionResult>& events, const std::vector<SessionInfo>& sessions, const QDate& day) {
    day_ = day;
    std::fill(hourBuckets_.begin(), hourBuckets_.end(), BucketType::Empty);
    
    QDateTime dayStart(day, QTime(0, 0));
    QDateTime dayEnd = dayStart.addDays(1).addMSecs(-1);
    QDateTime now = QDateTime::currentDateTime();

    // Mark buckets as Benign if there's an active session during that time
    for (const auto& s : sessions) {
        QDateTime sEnd = s.endTime.isValid() ? s.endTime : now;
        if (s.startTime > dayEnd || sEnd < dayStart) continue; // No overlap

        QDateTime overlapStart = std::max(s.startTime, dayStart);
        QDateTime overlapEnd = std::min(sEnd, dayEnd);
        
        int startIdx = overlapStart.time().hour() * 2 + (overlapStart.time().minute() / 30);
        int endIdx = overlapEnd.time().hour() * 2 + (overlapEnd.time().minute() / 30);
        
        for (int i = startIdx; i <= endIdx && i < NUM_BUCKETS; ++i) {
            hourBuckets_[i] = BucketType::Benign;
        }
    }

    // Overwrite with Attack if there's an attack
    for (const auto& e : events) {
        if (e.timestamp.date() != day) continue;
        int hour = e.timestamp.time().hour();
        int min = e.timestamp.time().minute();
        int idx = hour * 2 + (min / 30);
        if (idx < NUM_BUCKETS) {
            if (e.label == 1) hourBuckets_[idx] = BucketType::Attack;
        }
    }
    update();
}

void TimelineWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    // Draw background using theme palette
    p.setBrush(ThemePalette::mantle());
    p.setPen(QPen(ThemePalette::surface0(), 1));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -25), 6, 6);

    double marginL = 8.0;
    double usableW = (double)width() - marginL - 8.0;
    int h = height() - 35;

    for (int i = 0; i < NUM_BUCKETS; i++) {
        double x1 = marginL + (usableW * i / NUM_BUCKETS);
        double x2 = marginL + (usableW * (i + 1) / NUM_BUCKETS);
        QRectF r(x1, 6, x2 - x1 - 2, h);
        
        QColor color = ThemePalette::surface0(); // Empty / No data
        if (hourBuckets_[i] == BucketType::Benign) color = ThemePalette::success();
        else if (hourBuckets_[i] == BucketType::Attack) color = ThemePalette::danger();
        
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(r, 2, 2);
    }

    p.setPen(ThemePalette::subtext0());
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
    exportPdfBtn_ = new QPushButton("Generate PDF Report");
    
    refreshBtn_->setToolTip("Обновить историю инцидентов из базы данных");
    exportBtn_->setToolTip("Экспортировать текущую таблицу в формате CSV");
    exportPdfBtn_->setToolTip("Сгенерировать подробный PDF-отчет по выбранному инциденту");
    
    topRow->addWidget(refreshBtn_); 
    topRow->addWidget(exportBtn_);
    topRow->addWidget(exportPdfBtn_);
    topRow->addStretch();
    
    QLabel* dateLabel = new QLabel("Date:");
    dateLabel->setProperty("cssClass", "statusLabelText");
    topRow->addWidget(dateLabel);
    
    dateEdit_ = new QDateEdit(QDate::currentDate());
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setMinimumWidth(110);
    dateEdit_->setToolTip("Выберите дату для просмотра истории инцидентов");
    topRow->addWidget(dateEdit_);

    filterType_ = new QComboBox();
    filterType_->addItems({"All Types", "DDoS Attack"});
    filterType_->setToolTip("Фильтр по типу события (Атака / Все)");
    topRow->addWidget(filterType_);

    ipFilter_ = new QLineEdit();
    ipFilter_->setPlaceholderText("Filter by IP...");
    ipFilter_->setToolTip("Быстрый поиск по IP-адресу в истории инцидентов");
    topRow->addWidget(ipFilter_);
    layout->addLayout(topRow);

    timeline_ = new TimelineWidget();
    QLabel* timelineLabel = new QLabel("Uptime / Threat Timeline (24h)");
    timelineLabel->setProperty("cssClass", "statusLabelText");
    timelineLabel->setToolTip("Временная шкала инцидентов (Зеленый - норма, Красный - атака, Серый - нет данных)");
    timeline_->setToolTip("Временная шкала инцидентов (Зеленый - норма, Красный - атака, Серый - нет данных)");
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
    layout->addWidget(table_);

    connect(refreshBtn_, &QPushButton::clicked, this, &EventHistoryWidget::refreshData);
    connect(exportBtn_, &QPushButton::clicked, this, &EventHistoryWidget::exportToCsv);
    connect(exportPdfBtn_, &QPushButton::clicked, this, &EventHistoryWidget::exportToPdf);
    connect(dateEdit_, &QDateEdit::dateChanged, this, &EventHistoryWidget::refreshData);
    connect(filterType_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &EventHistoryWidget::applyFilter);
    connect(ipFilter_, &QLineEdit::textChanged, this, &EventHistoryWidget::applyFilter);
}

void EventHistoryWidget::setDatabaseManager(DatabaseManager* db) {
    dbManager_ = db;
    refreshData();
}

void EventHistoryWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
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
        table_->setItem(i, 2, new QTableWidgetItem(e.topTalkers.empty() ? "unknown" : QString::fromStdString(e.topTalkers[0].first)));
        table_->setItem(i, 3, new QTableWidgetItem(QString::number(e.pps, 'f', 1)));
        table_->setItem(i, 4, new QTableWidgetItem(e.modelName.empty() ? "DDoS Attack" : QString::fromStdString(e.modelName)));
        table_->setItem(i, 5, new QTableWidgetItem(QString::number(e.confidence, 'f', 2)));
    }
    auto sessions = dbManager_->getSessions();
    timeline_->setEvents(events, sessions, dateEdit_->date());
    applyFilter();
}

void EventHistoryWidget::applyFilter() {
    QString ip = ipFilter_->text().trimmed();
    int typeIdx = filterType_->currentIndex();

    for (int r = 0; r < table_->rowCount(); r++) {
        bool show = true;
        if (!ip.isEmpty() && !table_->item(r, 2)->text().contains(ip)) show = false;
        // Adjust filter mapping based on your app's categories if needed
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
            out << CSVUtils::sanitizeField(table_->item(r, c)->text());
        }
        out << "\n";
    }
    f.close();
}

void EventHistoryWidget::exportToPdf() {
    auto selectedItems = table_->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, "Export Error", "Please select an incident from the table to generate a report.");
        return;
    }

    int row = selectedItems.first()->row();
    IncidentReportData data;
    data.startTime = table_->item(row, 0)->text();
    data.duration = table_->item(row, 1)->text();
    data.attackerIp = table_->item(row, 2)->text();
    data.maxPps = table_->item(row, 3)->text();
    data.type = table_->item(row, 4)->text();
    data.confidence = table_->item(row, 5)->text();

    QString defaultName = QString("Report_%1.pdf").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    QString path = QFileDialog::getSaveFileName(this, "Save PDF Report", defaultName, "PDF files (*.pdf)");
    if (path.isEmpty()) return;

    if (ReportGenerator::generatePdf(path, data)) {
        QMessageBox::information(this, "Success", "PDF report generated successfully!");
    } else {
        QMessageBox::critical(this, "Error", "Failed to generate PDF report.");
    }
}
