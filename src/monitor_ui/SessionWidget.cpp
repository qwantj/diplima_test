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
    table_->verticalHeader()->setVisible(false);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setShowGrid(false);
    table_->setSortingEnabled(true);
    table_->setStyleSheet(R"(
        QTableWidget {
            background: #2b2b2b; color: #eeeeee;
            border: none; font-size: 12px; gridline-color: transparent;
        }
        QTableWidget::item { padding: 8px 12px; border-bottom: 1px solid #3d3d3d; }
        QTableWidget::item:selected { background: #444444; }
        QHeaderView::section {
            background: #222222; color: #aaaaaa;
            border: none; border-bottom: 1px solid #444444;
            padding: 8px 12px; font-weight: bold; font-size: 12px;
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
        auto* idItem = new QTableWidgetItem(); idItem->setData(Qt::DisplayRole, s.id);
        table_->setItem(i, 0, idItem);
        table_->setItem(i, 1, new QTableWidgetItem(s.startTime.toString("yyyy-MM-dd HH:mm:ss")));
        table_->setItem(i, 2, new QTableWidgetItem(s.interfaceName));
        table_->setItem(i, 3, new QTableWidgetItem(s.modelName));
        auto* attackItem = new QTableWidgetItem(); attackItem->setData(Qt::DisplayRole, (qlonglong)s.totalAttacks);
        if (s.totalAttacks > 0) attackItem->setForeground(QColor(255, 100, 100));
        table_->setItem(i, 4, attackItem);
        auto* pktItem = new QTableWidgetItem(); pktItem->setData(Qt::DisplayRole, (qlonglong)s.totalPackets);
        table_->setItem(i, 5, pktItem);
    }

    table_->setSortingEnabled(true);
    table_->sortByColumn(0, Qt::DescendingOrder);
    table_->resizeColumnsToContents();
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
}
