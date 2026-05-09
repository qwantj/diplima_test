#include "monitor_ui/SessionWidget.hpp"
#include <QHeaderView>
#include <QLabel>

SessionWidget::SessionWidget(QWidget* parent) : QWidget(parent) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    table_ = new QTableWidget(this);
    table_->setColumnCount(6);
    table_->setHorizontalHeaderLabels({"ID", "Start Time", "Interface/PCAP", "Model", "Attacks", "Total Packets"});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table_->verticalHeader()->setVisible(false);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(false);
    table_->setShowGrid(false);
    table_->setSortingEnabled(true);
    table_->setStyleSheet(R"(
        QTableWidget {
            background: #1e1e2e; color: #cdd6f4;
            border: none; font-size: 12px;
            gridline-color: transparent;
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

    connect(table_, &QTableWidget::cellDoubleClicked, [this](int row, int) {
        auto* item = table_->item(row, 0);
        if (item) emit sessionSelected(item->text().toInt());
    });
}

void SessionWidget::loadSessions(const std::vector<SessionInfo>& sessions) {
    table_->setSortingEnabled(false);
    table_->setRowCount((int)sessions.size());

    for (int i = 0; i < (int)sessions.size(); i++) {
        const auto& s = sessions[i];

        // ID (numeric sort)
        auto* idItem = new QTableWidgetItem();
        idItem->setData(Qt::DisplayRole, s.id);
        table_->setItem(i, 0, idItem);

        // Start Time
        table_->setItem(i, 1, new QTableWidgetItem(s.startTime.toString("yyyy-MM-dd HH:mm:ss")));

        // Interface/PCAP
        table_->setItem(i, 2, new QTableWidgetItem(s.interfaceName));

        // Model
        table_->setItem(i, 3, new QTableWidgetItem(s.modelName));

        // Attacks (red if > 0)
        auto* attackItem = new QTableWidgetItem();
        attackItem->setData(Qt::DisplayRole, (qlonglong)s.totalAttacks);
        if (s.totalAttacks > 0) {
            attackItem->setForeground(QColor("#f38ba8")); // red
        }
        table_->setItem(i, 4, attackItem);

        // Total Packets
        auto* pktItem = new QTableWidgetItem();
        pktItem->setData(Qt::DisplayRole, (qlonglong)s.totalPackets);
        table_->setItem(i, 5, pktItem);
    }

    table_->setSortingEnabled(true);
    table_->sortByColumn(0, Qt::DescendingOrder);
    table_->resizeColumnsToContents();
    // Stretch Interface/PCAP column
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
}
