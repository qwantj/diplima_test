#include "monitor_ui/LogWidget.hpp"
#include <QHeaderView>
#include <QLabel>
#include <QHBoxLayout>

LogWidget::LogWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    // Filter bar
    auto* filterLayout = new QHBoxLayout();
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All Events", "Benign Only", "Attack Only"});
    filterCombo_->setFixedWidth(140);
    filterCombo_->setStyleSheet("QComboBox { background: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 4px; padding: 4px 8px; }");
    filterLayout->addWidget(filterCombo_);
    filterLayout->addStretch();
    layout->addLayout(filterLayout);

    // Table
    table_ = new QTableWidget(this);
    table_->setColumnCount(6);
    table_->setHorizontalHeaderLabels({"Time", "Flow", "Label", "Confidence", "PPS", "Model"});
    table_->verticalHeader()->setVisible(false);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(false);
    table_->setShowGrid(false);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setStyleSheet(R"(
        QTableWidget {
            background: #1e1e2e; color: #cdd6f4;
            border: none; font-size: 12px;
        }
        QTableWidget::item { padding: 2px 6px; border-bottom: 1px solid #252535; }
        QTableWidget::item:selected { background: #313244; }
        QHeaderView::section {
            background: #181825; color: #a6adc8;
            border: none; border-bottom: 1px solid #313244;
            padding: 6px 8px; font-weight: bold; font-size: 12px;
        }
    )");

    // Column widths
    table_->setColumnWidth(0, 120); // Time
    table_->setColumnWidth(1, 200); // Flow
    table_->setColumnWidth(2, 60);  // Label
    table_->setColumnWidth(3, 80);  // Confidence
    table_->setColumnWidth(4, 70);  // PPS
    // Model stretches

    layout->addWidget(table_);

    // Filter logic
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int idx) {
        for (int row = 0; row < table_->rowCount(); row++) {
            auto* labelItem = table_->item(row, 2);
            if (!labelItem) continue;
            bool show = true;
            if (idx == 1 && labelItem->text() != "Benign") show = false;
            if (idx == 2 && labelItem->text() != "Attack") show = false;
            table_->setRowHidden(row, !show);
        }
    });
}

void LogWidget::addEvent(const DetectionResult& r) {
    // Check filter
    int filter = filterCombo_->currentIndex();
    if (filter == 1 && r.label != 0) return;
    if (filter == 2 && r.label != 1) return;

    appendRow(r);

    // Trim old rows
    while (table_->rowCount() > MAX_ROWS) {
        table_->removeRow(0);
    }

    // Auto-scroll to bottom
    table_->scrollToBottom();
}

void LogWidget::loadHistory(const std::vector<DetectionResult>& events) {
    table_->setRowCount(0);
    for (const auto& e : events) {
        appendRow(e);
    }
}

void LogWidget::appendRow(const DetectionResult& r) {
    int row = table_->rowCount();
    table_->insertRow(row);

    // Time (with milliseconds)
    table_->setItem(row, 0, new QTableWidgetItem(r.timestamp.toString("HH:mm:ss.zzz")));

    // Flow (empty for now, could show top talker)
    table_->setItem(row, 1, new QTableWidgetItem(""));

    // Label (colored)
    auto* labelItem = new QTableWidgetItem(r.label == 0 ? "Benign" : "Attack");
    labelItem->setForeground(r.label == 0 ? QColor("#a6e3a1") : QColor("#f38ba8"));
    table_->setItem(row, 2, labelItem);

    // Confidence
    table_->setItem(row, 3, new QTableWidgetItem(QString::number(r.confidence, 'f', 2)));

    // PPS
    table_->setItem(row, 4, new QTableWidgetItem(QString::number(r.pps, 'f', 1)));

    // Model
    table_->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(r.modelName)));
}
