#include "monitor_ui/DashboardWidget.hpp"
#include <QGridLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QtMath>
#include <algorithm>
#include <QHeaderView>

// ====== HeatmapWidget ======
HeatmapWidget::HeatmapWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(200, 150);
}

void HeatmapWidget::addData(const QDateTime& time, const std::map<uint16_t, uint64_t>& portData) {
    uint64_t maxVal = 1;
    for (auto& [p, c] : portData) maxVal = std::max(maxVal, c);
    for (auto& [p, c] : portData) points_.push_back({time, p, (double)c / maxVal});
    while(points_.size() > 500) points_.pop_front();
    if(!points_.empty()) { minTime_ = points_.front().t; maxTime_ = points_.back().t; }
    update();
}

void HeatmapWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), ThemePalette::cardBackground());
    if (points_.empty()) {
        p.setPen(ThemePalette::textSecondary());
        p.drawText(rect(), Qt::AlignCenter, "No heatmap data");
        return;
    }
    qint64 tRange = std::max((qint64)1, maxTime_.toMSecsSinceEpoch() - minTime_.toMSecsSinceEpoch());
    for (auto& pt : points_) {
        qint64 dt = pt.t.toMSecsSinceEpoch() - minTime_.toMSecsSinceEpoch();
        int x = 10 + (int)((double)dt / tRange * (width() - 20));
        int y = 10 + (pt.port % 100) * (height() - 20) / 100;
        QColor c = ThemePalette::heatmapHigh();
        c.setAlpha(std::min(255, (int)(pt.intensity * 255)));
        p.setPen(Qt::NoPen); p.setBrush(c);
        p.drawEllipse(QPoint(x, y), 3, 3);
    }
}

// ====== NetworkTopologyWidget ======
NetworkTopologyWidget::NetworkTopologyWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(300, 200);
}

void NetworkTopologyWidget::updateTopology(const std::vector<std::pair<std::string, uint64_t>>& targets, double ingressMbps) {
    targets_ = targets; if(targets_.size() > 5) targets_.resize(5);
    ingressMbps_ = ingressMbps; update();
}

void NetworkTopologyWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), ThemePalette::cardBackground());
    p.setPen(ThemePalette::textPrimary());
    p.drawText(QRect(10, 10, width()-20, 20), Qt::AlignCenter, QString("Ingress Traffic: %1 Mbps").arg(ingressMbps_, 0, 'f', 1));
    if (targets_.empty()) return;
    int monX = 50, monY = height() / 2;
    int tarX = width() - 100;
    int tarYStep = (height() - 40) / targets_.size();
    p.setBrush(QColor(100, 150, 255)); p.setPen(Qt::NoPen);
    p.drawEllipse(QPoint(monX, monY), 15, 15);
    p.setPen(ThemePalette::textSecondary());
    p.drawText(QRect(monX-20, monY+20, 40, 20), Qt::AlignCenter, "Monitor");
    for (size_t i = 0; i < targets_.size(); i++) {
        int ty = 20 + i * tarYStep + tarYStep/2;
        p.setPen(QPen(QColor(80, 80, 80), 1));
        p.drawLine(monX+15, monY, tarX-15, ty);
        p.setPen(ThemePalette::textSecondary());
        p.drawText(QRect((monX+tarX)/2 - 30, (monY+ty)/2 - 15, 60, 20), Qt::AlignCenter, QString("%1 pkts").arg(targets_[i].second));
        p.setBrush(ThemePalette::danger()); p.setPen(Qt::NoPen);
        p.drawEllipse(QPoint(tarX, ty), 10, 10);
        p.setPen(ThemePalette::textPrimary());
        p.drawText(QRect(tarX-40, ty+15, 80, 20), Qt::AlignCenter, QString::fromStdString(targets_[i].first));
    }
}

// ====== AlertGridWidget ======
AlertGridWidget::AlertGridWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(200, 80);
    healthHistory_.resize(32, true);
}

void AlertGridWidget::addHealthPoint(bool ok) {
    healthHistory_.push_back(ok);
    if(healthHistory_.size() > 32) healthHistory_.pop_front();
    update();
}

void AlertGridWidget::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    int cols = 8, rows = 4;
    int cellW = width() / cols, cellH = height() / rows;
    for (int i = 0; i < 32; i++) {
        int r = i / cols, c = i % cols;
        QRect rect(c * cellW + 2, r * cellH + 2, cellW - 4, cellH - 4);
        bool ok = healthHistory_[i];
        QColor color = ok ? ThemePalette::success() : ThemePalette::danger();
        p.setBrush(color); p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect, 3, 3);
    }
}

// ====== InteractiveChartView ======
InteractiveChartView::InteractiveChartView(QChart* chart, QWidget* parent)
    : QChartView(chart, parent) {
    setRubberBand(QChartView::RectangleRubberBand);
}

void InteractiveChartView::wheelEvent(QWheelEvent* event) {
    emit userInteracted();
    qreal factor = event->angleDelta().y() > 0 ? 1.1 : 0.9;
    chart()->zoom(factor); event->accept();
}

void InteractiveChartView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        dragging_ = true; lastMousePos_ = event->position();
        setCursor(Qt::ClosedHandCursor); emit userInteracted(); event->accept();
    } else QChartView::mousePressEvent(event);
}

void InteractiveChartView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        dragging_ = false; setCursor(Qt::ArrowCursor); event->accept();
    } else QChartView::mouseReleaseEvent(event);
}

void InteractiveChartView::mouseMoveEvent(QMouseEvent* event) {
    if (dragging_) {
        QPointF delta = event->position() - lastMousePos_;
        chart()->scroll(-delta.x(), delta.y()); lastMousePos_ = event->position();
        event->accept();
    } else QChartView::mouseMoveEvent(event);
}

// ====== DashboardWidget ======
DashboardWidget::DashboardWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    metricsTimer_ = new QTimer(this);
    connect(metricsTimer_, &QTimer::timeout, this, &DashboardWidget::updateSystemMetrics);
    metricsTimer_->start(2000);
    autoScrollTimer_ = new QTimer(this);
    autoScrollTimer_->setSingleShot(true);
    connect(autoScrollTimer_, &QTimer::timeout, [this]() { userInteracting_ = false; });
}

void DashboardWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    setupDashboard();
    mainLayout->addWidget(tabSystem_);
    setupAnalytics();
}

void DashboardWidget::setupDashboard() {
    tabSystem_ = new QWidget();
    auto* layout = new QVBoxLayout(tabSystem_);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* overviewWidget = new QWidget();
    overviewWidget->setStyleSheet("background: #222222; border-bottom: 1px solid #444444;");
    auto* overviewLayout = new QHBoxLayout(overviewWidget);
    overviewLayout->setContentsMargins(15, 10, 15, 10);
    overviewLayout->setSpacing(20);
    
    auto createStatusLabel = [](const QString& text, const QString& color = "#cccccc") {
        auto* l = new QLabel(text);
        l->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: 600;").arg(color));
        return l;
    };

    lblCollector_ = createStatusLabel("Collector: Disconnected", "#ff6666");
    lblTotalPackets_ = createStatusLabel("Total Packets: 0");
    lblPps_ = createStatusLabel("Current PPS: 0.0");
    lblDropRate_ = createStatusLabel("Drop Rate: 0.0 pps", "#ffaa66");
    lblSources_ = createStatusLabel("Sources: 0");
    bpfCheckbox_ = new QCheckBox("Auto-Block Top-IP (BPF)");
    bpfCheckbox_->setStyleSheet("QCheckBox { color: #cccccc; font-size: 12px; }");
    connect(bpfCheckbox_, &QCheckBox::toggled, this, &DashboardWidget::bpfToggled);
    
    lblModel_ = createStatusLabel("None", "#ff99ff");
    lblProbability_ = createStatusLabel("Probability: 0.0%", "#66ff66");
    lblStatus_ = createStatusLabel("✔ Normal Traffic", "#66ff66");
    lblStatus_->setStyleSheet(lblStatus_->styleSheet() + "font-weight: bold; font-size: 14px;");

    overviewLayout->addWidget(lblCollector_);
    overviewLayout->addWidget(lblTotalPackets_);
    overviewLayout->addWidget(lblPps_);
    overviewLayout->addWidget(lblDropRate_);
    overviewLayout->addWidget(lblSources_);
    overviewLayout->addWidget(bpfCheckbox_);
    overviewLayout->addStretch();
    overviewLayout->addWidget(lblModel_);
    overviewLayout->addWidget(lblProbability_);
    overviewLayout->addWidget(lblStatus_);
    layout->addWidget(overviewWidget);

    auto* legendWidget = new QWidget();
    legendWidget->setStyleSheet("background: #2b2b2b;");
    auto* legendLayout = new QHBoxLayout(legendWidget);
    legendLayout->setContentsMargins(20, 5, 20, 5);
    legendLayout->setSpacing(20);
    legendLayout->addStretch();
    auto createLayerCb = [&](const QString& text, const QColor& color, QCheckBox*& cb) {
        cb = new QCheckBox(text); cb->setChecked(true);
        cb->setStyleSheet(QString("QCheckBox { color: %1; font-weight: bold; font-size: 11px; }").arg(color.name()));
        legendLayout->addWidget(cb);
    };
    createLayerCb("Packets per Second", QColor(100, 200, 255), cbPps_);
    createLayerCb("Other", QColor(160, 160, 160), cbOther_);
    createLayerCb("ICMP", QColor(255, 100, 100), cbIcmp_);
    createLayerCb("UDP", QColor(255, 200, 100), cbUdp_);
    createLayerCb("TCP", QColor(100, 220, 100), cbTcp_);
    createLayerCb("Attack Confidence %", QColor(255, 150, 50), cbConf_);
    legendLayout->addStretch();
    layout->addWidget(legendWidget);

    ppsChart_ = new QChart();
    ppsChart_->setAnimationOptions(QChart::NoAnimation);
    ppsChart_->setBackgroundBrush(Qt::transparent);
    ppsChart_->legend()->hide();
    ppsChart_->setMargins(QMargins(0,0,0,0));
    ppsSeries_ = new QLineSeries(); tcpSeries_ = new QLineSeries();
    udpSeries_ = new QLineSeries(); icmpSeries_ = new QLineSeries(); otherSeries_ = new QLineSeries();
    attackConfidenceUpper_ = new QLineSeries();
    attackConfidenceArea_ = new QAreaSeries(attackConfidenceUpper_, new QLineSeries());
    ppsChart_->addSeries(ppsSeries_); ppsChart_->addSeries(otherSeries_);
    ppsChart_->addSeries(icmpSeries_); ppsChart_->addSeries(udpSeries_);
    ppsChart_->addSeries(tcpSeries_); ppsChart_->addSeries(attackConfidenceArea_);
    timeAxis_ = new QDateTimeAxis(); timeAxis_->setFormat("HH:mm:ss");
    timeAxis_->setGridLineColor(QColor(60, 60, 60));
    ppsChart_->addAxis(timeAxis_, Qt::AlignBottom);
    ppsAxis_ = new QValueAxis(); ppsAxis_->setGridLineColor(QColor(60, 60, 60));
    ppsChart_->addAxis(ppsAxis_, Qt::AlignLeft);
    confAxis_ = new QValueAxis(); confAxis_->setRange(0, 100); confAxis_->setGridLineVisible(false);
    ppsChart_->addAxis(confAxis_, Qt::AlignRight);
    ppsSeries_->attachAxis(timeAxis_); ppsSeries_->attachAxis(ppsAxis_);
    tcpSeries_->attachAxis(timeAxis_); tcpSeries_->attachAxis(ppsAxis_);
    udpSeries_->attachAxis(timeAxis_); udpSeries_->attachAxis(ppsAxis_);
    icmpSeries_->attachAxis(timeAxis_); icmpSeries_->attachAxis(ppsAxis_);
    otherSeries_->attachAxis(timeAxis_); otherSeries_->attachAxis(ppsAxis_);
    attackConfidenceArea_->attachAxis(timeAxis_); attackConfidenceArea_->attachAxis(confAxis_);
    connect(cbPps_, &QCheckBox::toggled, ppsSeries_, &QLineSeries::setVisible);
    connect(cbTcp_, &QCheckBox::toggled, tcpSeries_, &QLineSeries::setVisible);
    connect(cbUdp_, &QCheckBox::toggled, udpSeries_, &QLineSeries::setVisible);
    connect(cbIcmp_, &QCheckBox::toggled, icmpSeries_, &QLineSeries::setVisible);
    connect(cbOther_, &QCheckBox::toggled, otherSeries_, &QLineSeries::setVisible);
    connect(cbConf_, &QCheckBox::toggled, attackConfidenceArea_, &QAreaSeries::setVisible);
    ppsChartView_ = new InteractiveChartView(ppsChart_, this);
    ppsChartView_->setRenderHint(QPainter::Antialiasing);
    ppsChartView_->setStyleSheet("background: #1e1e1e; border: none;");
    connect(ppsChartView_, &InteractiveChartView::userInteracted, this, &DashboardWidget::onUserInteracted);
    
    auto* chartWrapper = new QWidget();
    auto* wrapperLayout = new QVBoxLayout(chartWrapper);
    wrapperLayout->setContentsMargins(0, 0, 0, 0); wrapperLayout->setSpacing(0);
    auto* resetLayout = new QHBoxLayout(); resetLayout->addStretch();
    auto* resetZoomBtn = new QPushButton("Reset Zoom");
    resetZoomBtn->setStyleSheet("background: #444444; color: #eeeeee; border-radius: 4px; padding: 4px 12px; border: none; margin: 5px;");
    connect(resetZoomBtn, &QPushButton::clicked, this, &DashboardWidget::resetZoom);
    resetLayout->addWidget(resetZoomBtn); wrapperLayout->addLayout(resetLayout);
    wrapperLayout->addWidget(ppsChartView_, 1);
    layout->addWidget(chartWrapper, 1);

    auto* donutsLayout = new QHBoxLayout();
    donutsLayout->setSpacing(15); donutsLayout->setContentsMargins(15, 10, 15, 15);
    auto createDonut = [&](QChart*& chart, QPieSeries*& pie, QLabel*& titleLbl, const QString& titleText) {
        auto* container = new QWidget(); container->setStyleSheet("background: #2b2b2b; border-radius: 12px; border: 1px solid #444444;");
        auto* l = new QVBoxLayout(container); l->setContentsMargins(15, 15, 15, 15);
        titleLbl = new QLabel(titleText); titleLbl->setAlignment(Qt::AlignCenter);
        titleLbl->setStyleSheet("color: #eeeeee; font-weight: bold; font-size: 14px;");
        l->addWidget(titleLbl);
        chart = new QChart(); chart->setAnimationOptions(QChart::NoAnimation);
        chart->setBackgroundBrush(Qt::transparent); chart->legend()->setAlignment(Qt::AlignBottom);
        chart->legend()->setLabelBrush(QBrush(QColor(160, 160, 160))); chart->setMargins(QMargins(0,0,0,0));
        pie = new QPieSeries(); pie->setHoleSize(0.65); chart->addSeries(pie);
        auto* v = new QChartView(chart); v->setRenderHint(QPainter::Antialiasing); v->setFixedHeight(180);
        v->setBackgroundBrush(Qt::transparent); l->addWidget(v);
        return container;
    };
    donutsLayout->addWidget(createDonut(cpuChart_, cpuPie_, cpuTitle_, "CPU (0.0%)"));
    donutsLayout->addWidget(createDonut(ramChart_, ramPie_, ramTitle_, "RAM (0.0%)"));
    donutsLayout->addWidget(createDonut(trafficChart_, trafficPie_, trafficTitle_, "Traffic Ratio (Attack: 0.0%)"));
    cpuPie_->append("In Use", 0)->setBrush(QColor(255, 100, 100));
    cpuPie_->append("Free", 100)->setBrush(QColor(100, 220, 100));
    ramPie_->append("In Use", 0)->setBrush(QColor(255, 150, 50));
    ramPie_->append("Free", 100)->setBrush(QColor(100, 220, 100));
    trafficPie_->append("Normal", 1)->setBrush(QColor(100, 220, 100));
    trafficPie_->append("Attack", 0)->setBrush(QColor(255, 100, 100));
    layout->addLayout(donutsLayout);
    applyTheme(ThemeMode::Dark);
}

void DashboardWidget::setupAnalytics() {
    tabAnalytics_ = new QWidget();
    auto* layout = new QVBoxLayout(tabAnalytics_);
    auto* topLayout = new QHBoxLayout();
    auto* healthGrp = new QGroupBox("Global SLO Health (Last points)");
    healthGrp->setStyleSheet("color: #eeeeee; font-weight: bold;");
    auto* hL = new QVBoxLayout(healthGrp); sloHealth_ = new AlertGridWidget(); hL->addWidget(sloHealth_);
    topLayout->addWidget(healthGrp, 1);
    auto* topGrp = new QGroupBox("Network Topology");
    topGrp->setStyleSheet("color: #eeeeee; font-weight: bold;");
    auto* tL = new QVBoxLayout(topGrp); topologyWidget_ = new NetworkTopologyWidget(); tL->addWidget(topologyWidget_);
    topLayout->addWidget(topGrp, 2);
    layout->addLayout(topLayout, 1);
    auto* midLayout = new QHBoxLayout();
    auto setupTable = [](QTableWidget*& t, const QStringList& headers) {
        t = new QTableWidget(); t->setColumnCount(headers.size()); t->setHorizontalHeaderLabels(headers);
        t->verticalHeader()->setVisible(false); t->horizontalHeader()->setStretchLastSection(true);
        t->setEditTriggers(QAbstractItemView::NoEditTriggers); t->setSelectionBehavior(QAbstractItemView::SelectRows);
        t->setStyleSheet("background: #2b2b2b; color: #eeeeee; border: none; gridline-color: #444444;");
    };
    setupTable(tableSources_, {"#", "IP Addr", "Packets"}); midLayout->addWidget(tableSources_, 1);
    setupTable(tableTargets_, {"#", "Target (IP:Port)", "Packets"}); midLayout->addWidget(tableTargets_, 1);
    auto* portsGrp = new QGroupBox("Top Active Ports");
    portsGrp->setStyleSheet("color: #eeeeee; font-weight: bold;");
    auto* pL = new QVBoxLayout(portsGrp);
    topPortsChart_ = new QChart(); topPortsChart_->setBackgroundBrush(Qt::transparent);
    topPortsChart_->legend()->setAlignment(Qt::AlignBottom); topPortsChart_->legend()->setLabelBrush(QBrush(QColor(160, 160, 160)));
    topPortsPie_ = new QPieSeries(); topPortsPie_->setHoleSize(0.4); topPortsChart_->addSeries(topPortsPie_);
    auto* pv = new QChartView(topPortsChart_); pv->setRenderHint(QPainter::Antialiasing); pL->addWidget(pv);
    midLayout->addWidget(portsGrp, 1);
    layout->addLayout(midLayout, 2);
    auto* botLayout = new QHBoxLayout();
    auto* bwGrp = new QGroupBox("Network Bandwidth"); bwGrp->setStyleSheet("color: #eeeeee; font-weight: bold;");
    auto* bL = new QVBoxLayout(bwGrp); bandwidthChart_ = new QChart(); bandwidthChart_->setBackgroundBrush(Qt::transparent);
    bandwidthChart_->legend()->hide(); bandwidthSeries_ = new QLineSeries(); bandwidthSeries_->setPen(QPen(QColor(100, 200, 255), 2));
    bandwidthChart_->addSeries(bandwidthSeries_); bwTimeAxis_ = new QDateTimeAxis(); bwTimeAxis_->setFormat("HH:mm");
    bwAxis_ = new QValueAxis(); bwAxis_->setTitleText("Mbps");
    bandwidthChart_->addAxis(bwTimeAxis_, Qt::AlignBottom); bandwidthChart_->addAxis(bwAxis_, Qt::AlignLeft);
    bandwidthSeries_->attachAxis(bwTimeAxis_); bandwidthSeries_->attachAxis(bwAxis_);
    auto* bv = new QChartView(bandwidthChart_); bv->setRenderHint(QPainter::Antialiasing); bL->addWidget(bv);
    botLayout->addWidget(bwGrp, 1);
    auto* hmGrp = new QGroupBox("Port Activity Heatmap"); hmGrp->setStyleSheet("color: #eeeeee; font-weight: bold;");
    auto* hmL = new QVBoxLayout(hmGrp); heatmapWidget_ = new HeatmapWidget(); hmL->addWidget(heatmapWidget_);
    botLayout->addWidget(hmGrp, 1);
    auto* sizeGrp = new QGroupBox("Packet Size Distribution"); sizeGrp->setStyleSheet("color: #eeeeee; font-weight: bold;");
    auto* sL = new QVBoxLayout(sizeGrp); packetSizeChart_ = new QChart(); packetSizeChart_->setBackgroundBrush(Qt::transparent);
    packetSizeChart_->legend()->hide(); packetSizeSeries_ = new QBarSeries(); packetSizeSet_ = new QBarSet("Count");
    packetSizeSet_->setColor(QColor(100, 200, 255)); packetSizeSeries_->append(packetSizeSet_);
    packetSizeChart_->addSeries(packetSizeSeries_); sizeCatAxis_ = new QBarCategoryAxis();
    sizeCatAxis_->append({"0-64B", "65-128B", "129-512B", "513-1024B", ">1024B"}); sizeValAxis_ = new QValueAxis();
    packetSizeChart_->addAxis(sizeCatAxis_, Qt::AlignBottom); packetSizeChart_->addAxis(sizeValAxis_, Qt::AlignLeft);
    packetSizeSeries_->attachAxis(sizeCatAxis_); packetSizeSeries_->attachAxis(sizeValAxis_);
    auto* sv = new QChartView(packetSizeChart_); sv->setRenderHint(QPainter::Antialiasing); sL->addWidget(sv);
    botLayout->addWidget(sizeGrp, 1);
    layout->addLayout(botLayout, 2);
}

void DashboardWidget::applyTheme(ThemeMode mode) {
    ThemePalette::apply(mode);
    ppsSeries_->setPen(QPen(QColor(100, 200, 255), 2));
    tcpSeries_->setPen(QPen(QColor(100, 220, 100), 1.5));
    udpSeries_->setPen(QPen(QColor(255, 200, 100), 1.5));
    icmpSeries_->setPen(QPen(QColor(255, 100, 100), 1.5));
    otherSeries_->setPen(QPen(QColor(160, 160, 160), 1.5));
    QColor attackColor = QColor(255, 150, 50); attackColor.setAlpha(80);
    attackConfidenceArea_->setBrush(attackColor); attackConfidenceArea_->setPen(QPen(QColor(255, 150, 50), 1));
    timeAxis_->setLabelsColor(ThemePalette::textSecondary());
    ppsAxis_->setLabelsColor(ThemePalette::textSecondary());
    confAxis_->setLabelsColor(ThemePalette::textSecondary());
}

void DashboardWidget::resetZoom() { ppsChart_->zoomReset(); userInteracting_ = false; }
QWidget* DashboardWidget::getTabAnalytics() const { return tabAnalytics_; }
QWidget* DashboardWidget::getTabSystem() const { return tabSystem_; }
void DashboardWidget::onUserInteracted() { userInteracting_ = true; autoScrollTimer_->start(10000); }

void DashboardWidget::updateRealtime(const DetectionResult& result, uint64_t totalPackets) {
    addDataPoint(result);
    if (result.label == 1) totalAttack_++; else totalNormal_++;
    lblTotalPackets_->setText(QString("Total Packets: %1").arg(totalPackets));
    lblPps_->setText(QString("Current PPS: %1").arg(result.pps, 0, 'f', 1));
    lblSources_->setText(QString("Sources: %1").arg(result.uniqueSourceCount));
    lblDropRate_->setText(QString("Drop Rate: %1 pps").arg(result.droppedPackets));
    lblDropRate_->setToolTip(QString("Total Dropped: %1 | Queue Size: %2").arg(result.droppedPackets).arg(result.queueSize));
    lblModel_->setText("🧠 " + QString::fromStdString(result.modelName));
    lblModel_->setToolTip(QString("Inference Latency: %1 ms").arg(result.inferenceLatencyMs, 0, 'f', 2));
    lblProbability_->setText(QString("Probability: %1%").arg(result.confidence * 100, 0, 'f', 1));
    if (result.label == 1) {
        lblProbability_->setStyleSheet("color: #ff6666; font-weight: bold;");
        lblStatus_->setText("⚠ ATTACK DETECTED"); lblStatus_->setStyleSheet("color: #ff6666; font-weight: bold;");
    } else {
        lblProbability_->setStyleSheet("color: #66ff66; font-weight: bold;");
        lblStatus_->setText("✔ Normal Traffic"); lblStatus_->setStyleSheet("color: #66ff66; font-weight: bold;");
    }
    sloHealth_->addHealthPoint(result.label == 0);
    double ingressMbps = (result.totalBytes * 8.0) / 1000000.0 / (result.flowDuration > 0 ? result.flowDuration : 1.0);
    topologyWidget_->updateTopology(result.topTargets, ingressMbps);
    if (!result.topTalkers.empty()) {
        tableSources_->setRowCount(std::min(5, (int)result.topTalkers.size()));
        for(int i=0; i<tableSources_->rowCount(); ++i) {
            tableSources_->setItem(i, 0, new QTableWidgetItem(QString::number(i+1)));
            tableSources_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(result.topTalkers[i].first)));
            tableSources_->setItem(i, 2, new QTableWidgetItem(QString::number(result.topTalkers[i].second)));
        }
    }
    if (!result.topTargets.empty()) {
        tableTargets_->setRowCount(std::min(5, (int)result.topTargets.size()));
        for(int i=0; i<tableTargets_->rowCount(); ++i) {
            tableTargets_->setItem(i, 0, new QTableWidgetItem(QString::number(i+1)));
            tableTargets_->setItem(i, 1, new QTableWidgetItem(QString::fromStdString(result.topTargets[i].first)));
            tableTargets_->setItem(i, 2, new QTableWidgetItem(QString::number(result.topTargets[i].second)));
        }
    }
    if (!result.topPorts.empty()) {
        topPortsPie_->clear(); std::map<uint16_t, uint64_t> portMap;
        int colors[] = {0x6699ff, 0xffcc66, 0x66ff99, 0xff6666, 0xcc99ff};
        for (int i=0; i<std::min(5, (int)result.topPorts.size()); ++i) {
            auto slice = topPortsPie_->append(QString::number(result.topPorts[i].first), result.topPorts[i].second);
            slice->setBrush(QColor(colors[i%5])); portMap[result.topPorts[i].first] = result.topPorts[i].second;
        }
        heatmapWidget_->addData(result.timestamp, portMap);
    }
    if (result.packetSizeHistogram.size() == 5) {
        for(int i=0; i<5; ++i) packetSizeSet_->replace(i, result.packetSizeHistogram[i]);
        double m = 1; for(int x : result.packetSizeHistogram) m = std::max(m, (double)x);
        sizeValAxis_->setRange(0, m * 1.1);
    }
}

void DashboardWidget::addDataPoint(const DetectionResult& result) {
    QDateTime t = result.timestamp.isValid() ? result.timestamp : QDateTime::currentDateTime();
    qreal ms = t.toMSecsSinceEpoch();
    ppsSeries_->append(ms, result.pps);
    double dt = result.flowDuration > 0 ? result.flowDuration : 1.0;
    tcpSeries_->append(ms, result.tcpPackets / dt); udpSeries_->append(ms, result.udpPackets / dt);
    icmpSeries_->append(ms, result.icmpPackets / dt); otherSeries_->append(ms, result.otherPackets / dt);
    attackConfidenceUpper_->append(ms, result.confidence * 100.0);
    bandwidthSeries_->append(ms, (result.totalBytes * 8.0) / 1000000.0 / dt);
    timeHistory_.push_back(t);
    while (timeHistory_.size() > 300) {
        ppsSeries_->remove(0); tcpSeries_->remove(0); udpSeries_->remove(0);
        icmpSeries_->remove(0); otherSeries_->remove(0); attackConfidenceUpper_->remove(0);
        bandwidthSeries_->remove(0); timeHistory_.pop_front();
    }
    if (!userInteracting_ && !timeHistory_.empty()) {
        auto first = timeHistory_.front(); auto last = timeHistory_.back();
        timeAxis_->setRange(first, last.addSecs(2)); bwTimeAxis_->setRange(first, last.addSecs(2));
        double maxPps = 10; for (auto& pt : ppsSeries_->points()) maxPps = std::max(maxPps, pt.y());
        ppsAxis_->setRange(0, maxPps * 1.2);
        double maxBw = 1; for (auto& pt : bandwidthSeries_->points()) maxBw = std::max(maxBw, pt.y());
        bwAxis_->setRange(0, maxBw * 1.2);
    }
}

void DashboardWidget::updateSnapshot(float pps, uint64_t totalPackets, int currentLabel) {
    lblPps_->setText(QString("Current PPS: %1").arg(pps, 0, 'f', 1));
    lblTotalPackets_->setText(QString("Total Packets: %1").arg(totalPackets));
}

void DashboardWidget::updateConnectionStatus(bool connected) {
    if (connected) { lblCollector_->setText("Collector: Connected (Live)"); lblCollector_->setStyleSheet("color: #66ff66; font-weight: bold;"); }
    else { lblCollector_->setText("Collector: Disconnected"); lblCollector_->setStyleSheet("color: #ff6666; font-weight: bold;"); }
}

void DashboardWidget::loadHistory(const std::vector<DetectionResult>& events) {
    ppsSeries_->clear(); tcpSeries_->clear(); udpSeries_->clear();
    icmpSeries_->clear(); otherSeries_->clear(); attackConfidenceUpper_->clear();
    bandwidthSeries_->clear(); timeHistory_.clear(); totalNormal_ = 0; totalAttack_ = 0;
    for (auto& e : events) { if(e.label == 1) totalAttack_++; else totalNormal_++; addDataPoint(e); }
}

void DashboardWidget::updateSystemMetrics() { auto m = metricsCollector_.collect(); updateDonuts(m.cpuUsagePercent, m.ramUsagePercent); }

void DashboardWidget::updateDonuts(double cpu, double ram) {
    cpuPie_->slices()[0]->setValue(cpu); cpuPie_->slices()[1]->setValue(100.0 - cpu);
    cpuTitle_->setText(QString("CPU (%1%)").arg(cpu, 0, 'f', 1));
    ramPie_->slices()[0]->setValue(ram); ramPie_->slices()[1]->setValue(100.0 - ram);
    ramTitle_->setText(QString("RAM (%1%)").arg(ram, 0, 'f', 1));
    uint64_t total = totalNormal_ + totalAttack_;
    double attPct = total > 0 ? (double)totalAttack_ / total * 100.0 : 0.0;
    trafficPie_->slices()[0]->setValue(totalNormal_); trafficPie_->slices()[1]->setValue(totalAttack_);
    trafficTitle_->setText(QString("Traffic Ratio (Attack: %1%)").arg(attPct, 0, 'f', 1));
}
