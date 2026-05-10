#include "monitor_ui/LogWidget.hpp"
#include <QHeaderView>
#include <QLabel>
#include <QHBoxLayout>

LogWidget::LogWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* filterLayout = new QHBoxLayout();
    filterLayout->setContentsMargins(10, 8, 10, 8);
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All Events", "Benign Only", "Attack Only"});
    filterCombo_->setFixedWidth(140);
    filterCombo_->setStyleSheet("QComboBox { background: #333333; color: #eeeeee; border: 1px solid #444444; border-radius: 4px; padding: 4px 8px; }");
    filterLayout->addWidget(filterCombo_);
    filterLayout->addStretch();
    layout->addLayout(filterLayout);

    table_ = new QTableWidget(this);
    table_->setColumnCount(6);
    table_->setHorizontalHeaderLabels({"Time", "Flow", "Label", "Confidence", "PPS", "Model"});
    table_->verticalHeader()->setVisible(false);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setShowGrid(false);
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setStyleSheet(R"(
        QTableWidget {
            background: #2b2b2b; color: #eeeeee;
            border: none; font-size: 12px; gridline-color: #444444;
        }
        QTableWidget::item { padding: 5px 10px; border-bottom: 1px solid #3d3d3d; }
        QTableWidget::item:selected { background: #444444; }
        QHeaderView::section {
            background: #222222; color: #aaaaaa;
            border: none; border-bottom: 1px solid #444444;
            padding: 8px 10px; font-weight: bold; font-size: 12px;
        }
    )");

    table_->setColumnWidth(0, 130);
    table_->setColumnWidth(1, 200);
    table_->setColumnWidth(2, 80);
    table_->setColumnWidth(3, 90);
    table_->setColumnWidth(4, 80);

    layout->addWidget(table_);

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
    int filter = filterCombo_->currentIndex();
    if (filter == 1 && r.label != 0) return;
    if (filter == 2 && r.label != 1) return;
    appendRow(r);
    while (table_->rowCount() > MAX_ROWS) table_->removeRow(0);
    table_->scrollToBottom();
}

void LogWidget::loadHistory(const std::vector<DetectionResult>& events) {
    table_->setRowCount(0);
    for (const auto& e : events) appendRow(e);
}

void LogWidget::appendRow(const DetectionResult& r) {
    int row = table_->rowCount();
    table_->insertRow(row);
    table_->setItem(row, 0, new QTableWidgetItem(r.timestamp.toString("HH:mm:ss.zzz")));
    table_->setItem(row, 1, new QTableWidgetItem(r.uniqueSourceCount > 0 ? QString("Sources: %1").arg(r.uniqueSourceCount) : ""));
    auto* labelItem = new QTableWidgetItem(r.label == 0 ? "Benign" : "Attack");
    labelItem->setForeground(r.label == 0 ? QColor(100, 220, 100) : QColor(255, 100, 100));
    table_->setItem(row, 2, labelItem);
    table_->setItem(row, 3, new QTableWidgetItem(QString::number(r.confidence, 'f', 3)));
    table_->setItem(row, 4, new QTableWidgetItem(QString::number(r.pps, 'f', 1)));
    table_->setItem(row, 5, new QTableWidgetItem(QString::fromStdString(r.modelName)));
}
