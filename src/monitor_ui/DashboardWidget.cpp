#include "monitor_ui/DashboardWidget.hpp"
#include <QGridLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QtMath>
#include <algorithm>

// ====== HeatmapWidget ======
HeatmapWidget::HeatmapWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(200, 120);
}

void HeatmapWidget::setData(const std::map<uint16_t, uint64_t>& portData) {
    data_.clear();
    uint64_t maxVal = 1;
    for (auto& [p, c] : portData) maxVal = std::max(maxVal, c);
    for (auto& [p, c] : portData)
        data_.emplace_back(p, (double)c / maxVal);
    std::sort(data_.begin(), data_.end());
    if (data_.size() > 20) data_.resize(20);
    update();
}

void HeatmapWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), ThemePalette::cardBackground());

    if (data_.empty()) {
        p.setPen(ThemePalette::textSecondary());
        p.drawText(rect(), Qt::AlignCenter, "No port data");
        return;
    }

    int cellW = width() / std::min((int)data_.size(), 10);
    int rows = ((int)data_.size() + 9) / 10;
    int cellH = height() / std::max(rows, 1);

    for (int i = 0; i < (int)data_.size(); i++) {
        int col = i % 10, row = i / 10;
        QRect r(col * cellW, row * cellH, cellW - 1, cellH - 1);
        double intensity = data_[i].second;

        QColor c = ThemePalette::heatmapLow();
        if (intensity > 0.5) c = ThemePalette::heatmapHigh();
        else if (intensity > 0.2) c = ThemePalette::heatmapMid();

        p.fillRect(r, c);
        p.setPen(ThemePalette::textPrimary());
        p.setFont(QFont("Segoe UI", 7));
        p.drawText(r, Qt::AlignCenter, QString::number(data_[i].first));
    }
}

// ====== AlertGridWidget ======
AlertGridWidget::AlertGridWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(200, 80);
    cells_ = {
        {"PPS", 0, 10000, false},
        {"Label", 0, 0.5, false},
        {"Confidence", 0, 0.8, false},
        {"CPU %", 0, 80, false},
        {"RAM %", 0, 90, false}
    };
}

void AlertGridWidget::updateMetrics(double pps, int label, float confidence,
                                     double cpuPercent, double ramPercent) {
    cells_[0].value = pps;     cells_[0].alert = pps > 10000;
    cells_[1].value = label;   cells_[1].alert = label == 1;
    cells_[2].value = confidence; cells_[2].alert = confidence > 0.8 && label == 1;
    cells_[3].value = cpuPercent; cells_[3].alert = cpuPercent > 80;
    cells_[4].value = ramPercent; cells_[4].alert = ramPercent > 90;
    update();
}

void AlertGridWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), ThemePalette::cardBackground());

    int cellW = width() / (int)cells_.size();
    for (int i = 0; i < (int)cells_.size(); i++) {
        QRect r(i * cellW, 0, cellW - 2, height());
        QColor bg = cells_[i].alert ? ThemePalette::danger() : ThemePalette::success();
        bg.setAlpha(80);
        p.fillRect(r, bg);
        p.setPen(cells_[i].alert ? ThemePalette::danger() : ThemePalette::textPrimary());
        p.setFont(QFont("Segoe UI", 8, QFont::Bold));
        p.drawText(r.adjusted(4, 4, -4, -r.height()/2), Qt::AlignLeft, cells_[i].name);
        p.setFont(QFont("Segoe UI", 10));
        p.drawText(r.adjusted(4, r.height()/3, -4, -4), Qt::AlignLeft,
            QString::number(cells_[i].value, 'f', 1));
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
    chart()->zoom(factor);
    event->accept();
}

void InteractiveChartView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        dragging_ = true;
        lastMousePos_ = event->position();
        setCursor(Qt::ClosedHandCursor);
        emit userInteracted();
        event->accept();
    } else {
        QChartView::mousePressEvent(event);
    }
}

void InteractiveChartView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        dragging_ = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else {
        QChartView::mouseReleaseEvent(event);
    }
}

void InteractiveChartView::mouseMoveEvent(QMouseEvent* event) {
    if (dragging_) {
        QPointF delta = event->position() - lastMousePos_;
        chart()->scroll(-delta.x(), delta.y());
        lastMousePos_ = event->position();
        event->accept();
    } else {
        QChartView::mouseMoveEvent(event);
    }
}

// ====== DashboardWidget ======
DashboardWidget::DashboardWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
    setupCharts();

    // System metrics timer
    metricsTimer_ = new QTimer(this);
    connect(metricsTimer_, &QTimer::timeout, this, &DashboardWidget::updateSystemMetrics);
    metricsTimer_->start(2000);

    // Auto-scroll timer
    autoScrollTimer_ = new QTimer(this);
    autoScrollTimer_->setSingleShot(true);
    connect(autoScrollTimer_, &QTimer::timeout, [this]() { userInteracting_ = false; });
}

void DashboardWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);

    // Summary cards
    auto* cardsLayout = new QHBoxLayout();

    auto createCard = [](const QString& title, QLabel*& valueLabel) -> QWidget* {
        auto* card = new QWidget();
        card->setStyleSheet("background: rgba(49,50,68,0.6); border-radius: 8px; padding: 8px;");
        auto* vbox = new QVBoxLayout(card);
        auto* lbl = new QLabel(title);
        lbl->setStyleSheet("color: #a6adc8; font-size: 11px;");
        valueLabel = new QLabel("—");
        valueLabel->setStyleSheet("color: #cdd6f4; font-size: 18px; font-weight: bold;");
        vbox->addWidget(lbl);
        vbox->addWidget(valueLabel);
        return card;
    };

    cardsLayout->addWidget(createCard("Packets/s", lblPps_));
    cardsLayout->addWidget(createCard("Status", lblLabel_));
    cardsLayout->addWidget(createCard("Confidence", lblConfidence_));
    cardsLayout->addWidget(createCard("Total Packets", lblTotalPackets_));

    // Connection status
    lblConnectionStatus_ = new QLabel("⚫ Disconnected");
    lblConnectionStatus_->setStyleSheet("color: #f38ba8; font-size: 12px; padding: 4px;");
    cardsLayout->addWidget(lblConnectionStatus_);

    mainLayout->addLayout(cardsLayout);

    // Alert grid
    alertGridWidget_ = new AlertGridWidget(this);
    alertGridWidget_->setFixedHeight(60);
    mainLayout->addWidget(alertGridWidget_);

    // Charts area
    auto* chartsLayout = new QHBoxLayout();

    // PPS chart (main)
    ppsChart_ = new QChart();
    ppsChart_->setAnimationOptions(QChart::NoAnimation);
    ppsChart_->setBackgroundBrush(Qt::transparent);
    ppsChart_->legend()->hide();

    ppsChartView_ = new InteractiveChartView(ppsChart_, this);
    ppsChartView_->setRenderHint(QPainter::Antialiasing);
    ppsChartView_->setMinimumHeight(250);
    connect(ppsChartView_, &InteractiveChartView::userInteracted,
            this, &DashboardWidget::onUserInteracted);

    chartsLayout->addWidget(ppsChartView_, 3);

    // Right panel: pie + heatmap
    auto* rightPanel = new QVBoxLayout();

    protocolChart_ = new QChart();
    protocolChart_->setAnimationOptions(QChart::SeriesAnimations);
    protocolChart_->setBackgroundBrush(Qt::transparent);
    protocolChart_->setTitle("Protocol Breakdown");
    protocolChartView_ = new QChartView(protocolChart_);
    protocolChartView_->setRenderHint(QPainter::Antialiasing);
    protocolChartView_->setMaximumWidth(280);
    rightPanel->addWidget(protocolChartView_);

    heatmapWidget_ = new HeatmapWidget(this);
    heatmapWidget_->setMaximumWidth(280);
    rightPanel->addWidget(heatmapWidget_);

    chartsLayout->addLayout(rightPanel, 1);
    mainLayout->addLayout(chartsLayout);

    // BPF + System metrics row
    auto* bottomLayout = new QHBoxLayout();
    bpfCheckbox_ = new QCheckBox("Enable BPF Filter");
    connect(bpfCheckbox_, &QCheckBox::toggled, this, &DashboardWidget::bpfToggled);
    bottomLayout->addWidget(bpfCheckbox_);

    lblCpu_ = new QLabel("CPU: —%");
    lblRam_ = new QLabel("RAM: —%");
    lblCpu_->setStyleSheet("color: #a6adc8;");
    lblRam_->setStyleSheet("color: #a6adc8;");
    bottomLayout->addStretch();
    bottomLayout->addWidget(lblCpu_);
    bottomLayout->addWidget(lblRam_);
    mainLayout->addLayout(bottomLayout);

    // Tab widgets (for separate tabs in main window)
    tabAnalytics_ = new QWidget();
    tabSystem_ = new QWidget();
}

void DashboardWidget::setupCharts() {
    // PPS series
    ppsSeries_ = new QLineSeries();
    ppsSeries_->setName("PPS");
    ppsSeries_->setPen(QPen(ThemePalette::chartPps(), 2));
    ppsChart_->addSeries(ppsSeries_);

    tcpSeries_ = new QLineSeries();
    tcpSeries_->setName("TCP");
    tcpSeries_->setPen(QPen(ThemePalette::chartTcp(), 1.5));
    ppsChart_->addSeries(tcpSeries_);

    udpSeries_ = new QLineSeries();
    udpSeries_->setName("UDP");
    udpSeries_->setPen(QPen(ThemePalette::chartUdp(), 1.5));
    ppsChart_->addSeries(udpSeries_);

    icmpSeries_ = new QLineSeries();
    icmpSeries_->setName("ICMP");
    icmpSeries_->setPen(QPen(ThemePalette::chartIcmp(), 1.5));
    ppsChart_->addSeries(icmpSeries_);

    // Attack area
    attackUpper_ = new QLineSeries();
    auto* lower = new QLineSeries();
    attackArea_ = new QAreaSeries(attackUpper_, lower);
    attackArea_->setName("Attack");
    QColor attackColor = ThemePalette::chartAttack();
    attackColor.setAlpha(40);
    attackArea_->setBrush(attackColor);
    attackArea_->setPen(QPen(Qt::transparent));
    ppsChart_->addSeries(attackArea_);

    // Axes
    timeAxis_ = new QDateTimeAxis();
    timeAxis_->setFormat("HH:mm:ss");
    timeAxis_->setTitleText("Time");
    timeAxis_->setLabelsColor(ThemePalette::textSecondary());
    ppsChart_->addAxis(timeAxis_, Qt::AlignBottom);

    ppsAxis_ = new QValueAxis();
    ppsAxis_->setTitleText("Packets/s");
    ppsAxis_->setLabelsColor(ThemePalette::textSecondary());
    ppsChart_->addAxis(ppsAxis_, Qt::AlignLeft);

    ppsSeries_->attachAxis(timeAxis_);
    ppsSeries_->attachAxis(ppsAxis_);
    tcpSeries_->attachAxis(timeAxis_);
    tcpSeries_->attachAxis(ppsAxis_);
    udpSeries_->attachAxis(timeAxis_);
    udpSeries_->attachAxis(ppsAxis_);
    icmpSeries_->attachAxis(timeAxis_);
    icmpSeries_->attachAxis(ppsAxis_);
    attackArea_->attachAxis(timeAxis_);
    attackArea_->attachAxis(ppsAxis_);

    // Protocol pie
    protocolPie_ = new QPieSeries();
    protocolPie_->append("TCP", 1)->setBrush(ThemePalette::chartTcp());
    protocolPie_->append("UDP", 1)->setBrush(ThemePalette::chartUdp());
    protocolPie_->append("ICMP", 1)->setBrush(ThemePalette::chartIcmp());
    protocolPie_->setHoleSize(0.4);
    protocolChart_->addSeries(protocolPie_);
}

void DashboardWidget::updateRealtime(const DetectionResult& result, uint64_t totalPackets) {
    addDataPoint(result);
    updateSummaryCards(result, totalPackets);
    updateProtocolPie(result);

    if (!result.topPorts.empty()) {
        std::map<uint16_t, uint64_t> portMap;
        for (auto& [port, cnt] : result.topPorts) portMap[port] = cnt;
        heatmapWidget_->setData(portMap);
    }

    alertGridWidget_->updateMetrics(result.pps, result.label, result.confidence, 0, 0);
}

void DashboardWidget::updateSnapshot(float pps, uint64_t totalPackets, int currentLabel) {
    if (lblPps_)
        lblPps_->setText(QString::number(pps, 'f', 0));
    if (lblTotalPackets_)
        lblTotalPackets_->setText(QString::number(totalPackets));
}

void DashboardWidget::updateConnectionStatus(bool connected) {
    if (connected) {
        lblConnectionStatus_->setText("🟢 Connected");
        lblConnectionStatus_->setStyleSheet("color: #a6e3a1; font-size: 12px; padding: 4px;");
    } else {
        lblConnectionStatus_->setText("⚫ Disconnected");
        lblConnectionStatus_->setStyleSheet("color: #f38ba8; font-size: 12px; padding: 4px;");
    }
}

void DashboardWidget::loadHistory(const std::vector<DetectionResult>& events) {
    ppsSeries_->clear();
    tcpSeries_->clear();
    udpSeries_->clear();
    icmpSeries_->clear();
    attackUpper_->clear();
    timeHistory_.clear();

    for (auto& e : events) addDataPoint(e);
}

void DashboardWidget::applyTheme(ThemeMode mode) {
    ThemePalette::apply(mode);
    ppsSeries_->setPen(QPen(ThemePalette::chartPps(), 2));
    tcpSeries_->setPen(QPen(ThemePalette::chartTcp(), 1.5));
    udpSeries_->setPen(QPen(ThemePalette::chartUdp(), 1.5));
    icmpSeries_->setPen(QPen(ThemePalette::chartIcmp(), 1.5));
    timeAxis_->setLabelsColor(ThemePalette::textSecondary());
    ppsAxis_->setLabelsColor(ThemePalette::textSecondary());

    QColor attackColor = ThemePalette::chartAttack();
    attackColor.setAlpha(40);
    attackArea_->setBrush(attackColor);
}

void DashboardWidget::resetZoom() {
    ppsChart_->zoomReset();
    userInteracting_ = false;
}

QWidget* DashboardWidget::getTabAnalytics() const { return tabAnalytics_; }
QWidget* DashboardWidget::getTabSystem() const { return tabSystem_; }

void DashboardWidget::onUserInteracted() {
    userInteracting_ = true;
    autoScrollTimer_->start(10000); // 10 sec auto-scroll timeout
}

void DashboardWidget::addDataPoint(const DetectionResult& result) {
    QDateTime t = result.timestamp.isValid() ? result.timestamp : QDateTime::currentDateTime();
    qreal ms = t.toMSecsSinceEpoch();

    ppsSeries_->append(ms, result.pps);

    double dt = result.flowDuration > 0 ? result.flowDuration : 2.0;
    tcpSeries_->append(ms, result.tcpPackets / dt);
    udpSeries_->append(ms, result.udpPackets / dt);
    icmpSeries_->append(ms, result.icmpPackets / dt);

    // Attack area: full height if attack, 0 if benign
    attackUpper_->append(ms, result.label == 1 ? result.pps : 0);

    timeHistory_.push_back(t);
    while ((int)ppsSeries_->count() > MAX_CHART_POINTS) {
        ppsSeries_->remove(0);
        tcpSeries_->remove(0);
        udpSeries_->remove(0);
        icmpSeries_->remove(0);
        attackUpper_->remove(0);
        timeHistory_.pop_front();
    }

    // Auto-scroll axis
    if (!userInteracting_ && !timeHistory_.empty()) {
        auto first = timeHistory_.front();
        auto last = timeHistory_.back();
        timeAxis_->setRange(first, last.addSecs(2));

        double maxPps = 100;
        for (auto& pt : ppsSeries_->points())
            maxPps = std::max(maxPps, pt.y());
        ppsAxis_->setRange(0, maxPps * 1.2);
    }
}

void DashboardWidget::updateSummaryCards(const DetectionResult& result, uint64_t totalPackets) {
    lblPps_->setText(QString::number(result.pps, 'f', 0));
    lblTotalPackets_->setText(QString::number(totalPackets));
    lblConfidence_->setText(QString::number(result.confidence, 'f', 3));

    if (result.label == 1) {
        lblLabel_->setText("⚠ ATTACK");
        lblLabel_->setStyleSheet("color: #f38ba8; font-size: 18px; font-weight: bold;");
    } else {
        lblLabel_->setText("✅ Benign");
        lblLabel_->setStyleSheet("color: #a6e3a1; font-size: 18px; font-weight: bold;");
    }
}

void DashboardWidget::updateProtocolPie(const DetectionResult& result) {
    if (!protocolPie_) return;
    auto slices = protocolPie_->slices();
    if (slices.size() >= 3) {
        slices[0]->setValue(std::max(1.0, (double)result.tcpPackets));
        slices[1]->setValue(std::max(1.0, (double)result.udpPackets));
        slices[2]->setValue(std::max(1.0, (double)result.icmpPackets));
    }
}

void DashboardWidget::updateSystemMetrics() {
    auto m = metricsCollector_.collect();
    lblCpu_->setText(QString("CPU: %1%").arg(m.cpuUsagePercent, 0, 'f', 1));
    lblRam_->setText(QString("RAM: %1%").arg(m.ramUsagePercent, 0, 'f', 1));
    alertGridWidget_->updateMetrics(0, 0, 0, m.cpuUsagePercent, m.ramUsagePercent);
}
