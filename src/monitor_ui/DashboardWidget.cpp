#include "monitor_ui/DashboardWidget.hpp"
#include <QGridLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QtMath>
#include <algorithm>
#include <QHeaderView>
#include <QStyledItemDelegate>

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
    setMouseTracking(true);
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
    } else {
        emit mouseMoved(event->pos());
        QChartView::mouseMoveEvent(event);
    }
}

void InteractiveChartView::leaveEvent(QEvent* event) {
    emit mouseLeft();
    QChartView::leaveEvent(event);
}

// ====== Smart Tooltip (Monium Style) ======
SmartTooltip::SmartTooltip(QWidget* parent) : QFrame(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setStyleSheet("QFrame { background: rgba(255, 255, 255, 240); border: 1px solid #ccc; border-radius: 4px; }");
    setFixedWidth(200);
    
    auto* l = new QVBoxLayout(this);
    l->setContentsMargins(10, 8, 10, 8);
    label_ = new QLabel();
    label_->setStyleSheet("color: #333; font-size: 11px; border: none; background: transparent;");
    label_->setTextFormat(Qt::RichText);
    l->addWidget(label_);
    hide();
}

void SmartTooltip::updateContent(const QString& title, const QList<std::pair<QColor, QString>>& items, const QString& footer) {
    QString html;
    html.reserve(256 + items.size() * 128);
    html += QString("<b style='font-size: 12px;'>%1</b><br/>").arg(title);
    html += "<table width='100%' style='margin-top: 5px;'>";
    for (const auto& item : items) {
        html += QString("<tr><td width='15'><span style='color: %1;'>■</span></td>"
                        "<td>%2</td></tr>").arg(item.first.name(), item.second);
    }
    html += "</table>";
    if (!footer.isEmpty()) {
        html += QString("<hr style='border: none; border-top: 1px solid #ddd; margin: 5px 0;'/>");
        html += QString("<b>%1</b>").arg(footer);
    }
    label_->setText(html);
    adjustSize();
}

// ====== DonutChartView ======
DonutChartView::DonutChartView(QChart* chart, QWidget* parent) : QChartView(chart, parent) {}

void DonutChartView::setCenterText(const QString& text) {
    if (centerText_ != text) {
        centerText_ = text;
        viewport()->update();
    }
}

void DonutChartView::paintEvent(QPaintEvent* event) {
    QChartView::paintEvent(event);
    if (!centerText_.isEmpty() && chart()) {
        QPainter p(viewport());
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(ThemePalette::textPrimary());
        QFont f = p.font();
        f.setPointSize(16);
        f.setBold(true);
        p.setFont(f);
        
        // Draw exactly in the center of the plot area
        QRectF plotArea = chart()->plotArea();
        p.drawText(plotArea, Qt::AlignCenter, centerText_);
    }
}

// ====== ProgressDelegate (Draws a bar in the background) ======
class ProgressDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        if (index.column() == 2) { // "Packets" column
            bool ok;
            double val = index.data().toDouble(&ok);
            if (ok) {
                // Find max in column to scale
                double maxVal = 1;
                for (int i = 0; i < index.model()->rowCount(); ++i) {
                    maxVal = std::max(maxVal, index.model()->index(i, 2).data().toDouble());
                }
                
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                QRect r = option.rect.adjusted(2, 4, -2, -4);
                double ratio = std::min(1.0, val / maxVal);
                
                QColor barColor = QColor(116, 199, 236); barColor.setAlpha(60);
                painter->setBrush(barColor);
                painter->setPen(Qt::NoPen);
                painter->drawRoundedRect(r.x(), r.y(), (int)(r.width() * ratio), r.height(), 4, 4);
                painter->restore();
            }
        }
        QStyledItemDelegate::paint(painter, option, index);
    }
};

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
    auto* mainGridLayout = new QGridLayout(tabSystem_);
    mainGridLayout->setContentsMargins(15, 5, 15, 15);
    mainGridLayout->setSpacing(15);

    // Helper to create a stylized card
    auto createCard = [](QWidget* parent = nullptr) {
        auto* card = new QFrame(parent);
        card->setStyleSheet(ThemePalette::cardStyleSheet());
        return card;
    };

    // Title label
    auto* titleLabel = new QLabel("Overview (Global State)");
    titleLabel->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: bold; background: transparent;").arg(ThemePalette::text().name()));
    mainGridLayout->addWidget(titleLabel, 0, 0, 1, 3);

    // 1. Overview Status Bar (single horizontal row)
    auto* overviewBar = new QWidget();
    overviewBar->setFixedHeight(32);
    overviewBar->setStyleSheet("background: transparent;");
    auto* overviewLayout = new QHBoxLayout(overviewBar);
    overviewLayout->setContentsMargins(5, 0, 5, 0);
    overviewLayout->setSpacing(15);

    auto createStatusLabel = [](const QString& text, const QColor& color) {
        auto* lbl = new QLabel(text);
        lbl->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold; background: transparent;").arg(color.name()));
        return lbl;
    };

    lblCollector_ = createStatusLabel("Collector: Disconnected", ThemePalette::red());
    lblTotalPackets_ = createStatusLabel("Total Packets: 0", ThemePalette::text());
    lblPps_ = createStatusLabel("Current PPS: 0.0", ThemePalette::text());
    lblDropRate_ = createStatusLabel("Drop Rate: 0.0 pps", ThemePalette::peach());
    lblSources_ = createStatusLabel("Sources: 0", ThemePalette::text());
    
    bpfCheckbox_ = new QCheckBox("Auto-Block Top-IP (Firewall)");
    bpfCheckbox_->setStyleSheet(QString("QCheckBox { color: %1; font-size: 12px; font-weight: bold; background: transparent; }").arg(ThemePalette::text().name()));
    connect(bpfCheckbox_, &QCheckBox::toggled, this, &DashboardWidget::bpfToggled);

    lblModel_ = createStatusLabel("\u25CF rf_model.onnx", ThemePalette::text());
    lblProbability_ = createStatusLabel("Probability: 0.0%", ThemePalette::green());
    lblStatus_ = createStatusLabel("\u2714 Normal Traffic", ThemePalette::green());

    overviewLayout->addWidget(lblCollector_);
    overviewLayout->addWidget(lblTotalPackets_);
    overviewLayout->addWidget(lblPps_);
    overviewLayout->addWidget(lblDropRate_);
    overviewLayout->addWidget(lblSources_);
    overviewLayout->addWidget(bpfCheckbox_);
    overviewLayout->addWidget(lblModel_);
    overviewLayout->addStretch();
    overviewLayout->addWidget(lblProbability_);
    overviewLayout->addWidget(lblStatus_);

    mainGridLayout->addWidget(overviewBar, 1, 0, 1, 3);

    // 2. Main Area Chart Card
    auto* chartCard = createCard();
    auto* chartCardLayout = new QVBoxLayout(chartCard);
    chartCardLayout->setContentsMargins(10, 10, 10, 10);
    chartCardLayout->setSpacing(5);

    auto* legendLayout = new QHBoxLayout();
    legendLayout->setContentsMargins(10, 0, 10, 0);
    legendLayout->setSpacing(15);
    
    auto* chartTitle = new QLabel("Traffic Rate & Attack Confidence");
    chartTitle->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold; border: none; background: transparent;").arg(ThemePalette::text().name()));
    legendLayout->addWidget(chartTitle);
    legendLayout->addStretch();

    auto createLayerCb = [&](const QString& text, const QColor& color, QCheckBox*& cb) {
        cb = new QCheckBox(text); cb->setChecked(true);
        cb->setStyleSheet(QString("QCheckBox { color: %1; font-weight: bold; font-size: 11px; border: none; background: transparent; }").arg(color.name()));
        legendLayout->addWidget(cb);
    };
    
    createLayerCb("PPS", ThemePalette::sapphire(), cbPps_);
    createLayerCb("Other", ThemePalette::subtext0(), cbOther_);
    createLayerCb("ICMP", ThemePalette::red(), cbIcmp_);
    createLayerCb("UDP", ThemePalette::yellow(), cbUdp_);
    createLayerCb("TCP", ThemePalette::green(), cbTcp_);
    createLayerCb("Confidence %", ThemePalette::peach(), cbConf_);
    
    resetBtn_ = new QPushButton("Reset Zoom");
    resetBtn_->setStyleSheet(ThemePalette::buttonStyleSheet());
    legendLayout->addWidget(resetBtn_);

    chartCardLayout->addLayout(legendLayout);

    ppsChart_ = new QChart();
    ppsChart_->setAnimationOptions(QChart::NoAnimation);
    ppsChart_->setBackgroundBrush(Qt::transparent);
    ppsChart_->legend()->hide();
    ppsChart_->setMargins(QMargins(0,0,0,0));
    
    zeroSeries_ = new QLineSeries();
    
    ppsSeries_ = new QLineSeries(); 
    ppsArea_ = new QAreaSeries(ppsSeries_, zeroSeries_);
    ppsArea_->setName("PPS"); ppsArea_->setColor(ThemePalette::sapphire()); ppsArea_->setOpacity(0.2);

    otherSeries_ = new QLineSeries();
    otherArea_ = new QAreaSeries(otherSeries_, zeroSeries_);
    otherArea_->setName("Other"); otherArea_->setColor(ThemePalette::mauve()); otherArea_->setOpacity(0.3);
    
    tcpUpper_ = new QLineSeries();   tcpArea_ = new QAreaSeries(tcpUpper_, zeroSeries_);
    udpUpper_ = new QLineSeries();   udpArea_ = new QAreaSeries(udpUpper_, tcpUpper_);
    icmpUpper_ = new QLineSeries();  icmpArea_ = new QAreaSeries(icmpUpper_, udpUpper_);
    
    attackConfidenceUpper_ = new QLineSeries();
    attackConfidenceUpper_->setName("Attack Confidence");
    QPen dashPen(ThemePalette::peach());
    dashPen.setWidth(2);
    dashPen.setStyle(Qt::DashLine);
    attackConfidenceUpper_->setPen(dashPen);
    
    // Z-Order: Areas at bottom, lines on top
    ppsChart_->addSeries(ppsArea_);
    ppsChart_->addSeries(otherArea_);
    ppsChart_->addSeries(tcpArea_);
    ppsChart_->addSeries(udpArea_);
    ppsChart_->addSeries(icmpArea_);
    ppsChart_->addSeries(attackConfidenceUpper_);
    
    timeAxis_ = new QDateTimeAxis(); timeAxis_->setFormat("HH:mm:ss");
    timeAxis_->setGridLineColor(ThemePalette::surface0()); timeAxis_->setLinePenColor(Qt::transparent);
    ppsChart_->addAxis(timeAxis_, Qt::AlignBottom);
    
    ppsAxis_ = new QValueAxis(); ppsAxis_->setGridLineColor(ThemePalette::surface0()); ppsAxis_->setLinePenColor(Qt::transparent);
    ppsAxis_->setRange(0, 100);
    ppsChart_->addAxis(ppsAxis_, Qt::AlignLeft);
    
    confAxis_ = new QValueAxis(); confAxis_->setRange(0, 100); confAxis_->setGridLineVisible(false); confAxis_->setLinePenColor(Qt::transparent);
    ppsChart_->addAxis(confAxis_, Qt::AlignRight);
    
    ppsArea_->attachAxis(timeAxis_); ppsArea_->attachAxis(ppsAxis_);
    otherArea_->attachAxis(timeAxis_); otherArea_->attachAxis(ppsAxis_);
    tcpArea_->attachAxis(timeAxis_); tcpArea_->attachAxis(ppsAxis_);
    udpArea_->attachAxis(timeAxis_); udpArea_->attachAxis(ppsAxis_);
    icmpArea_->attachAxis(timeAxis_); icmpArea_->attachAxis(ppsAxis_);
    attackConfidenceUpper_->attachAxis(timeAxis_); attackConfidenceUpper_->attachAxis(confAxis_);
    
    connect(cbPps_, &QCheckBox::toggled, ppsArea_, &QAreaSeries::setVisible);
    connect(cbTcp_, &QCheckBox::toggled, tcpArea_, &QAreaSeries::setVisible);
    connect(cbUdp_, &QCheckBox::toggled, udpArea_, &QAreaSeries::setVisible);
    connect(cbIcmp_, &QCheckBox::toggled, icmpArea_, &QAreaSeries::setVisible);
    connect(cbOther_, &QCheckBox::toggled, otherArea_, &QAreaSeries::setVisible);
    connect(cbConf_, &QCheckBox::toggled, attackConfidenceUpper_, &QLineSeries::setVisible);
    connect(resetBtn_, &QPushButton::clicked, this, &DashboardWidget::resetZoom);
    
    ppsChartView_ = new InteractiveChartView(ppsChart_, chartCard);
    ppsChartView_->setRenderHint(QPainter::Antialiasing);
    ppsChartView_->setStyleSheet("background: transparent; border: none;");
    connect(ppsChartView_, &InteractiveChartView::userInteracted, this, &DashboardWidget::onUserInteracted);
    connect(ppsChartView_, &InteractiveChartView::mouseMoved, this, &DashboardWidget::onChartHover);
    connect(ppsChartView_, &InteractiveChartView::mouseLeft, this, &DashboardWidget::onChartLeave);
    
    chartTooltip_ = new SmartTooltip(ppsChartView_);
    
    chartCardLayout->addWidget(ppsChartView_, 1);
    mainGridLayout->addWidget(chartCard, 2, 0, 1, 3); // Spans across 3 columns
    mainGridLayout->setRowStretch(2, 3); // Give the chart more vertical space

    // 3. Donuts Bottom (3 Columns)
    auto createDonutCard = [&](QChart*& chart, QPieSeries*& pie, DonutChartView*& view, QLabel*& titleLbl, const QString& titleText) {
        auto* card = createCard();
        auto* l = new QVBoxLayout(card);
        l->setContentsMargins(15, 15, 15, 15);
        
        titleLbl = new QLabel(titleText);
        titleLbl->setAlignment(Qt::AlignLeft);
        titleLbl->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 14px; border: none; background: transparent;").arg(ThemePalette::text().name()));
        l->addWidget(titleLbl);
        
        chart = new QChart(); chart->setAnimationOptions(QChart::NoAnimation);
        chart->setBackgroundBrush(Qt::transparent); chart->legend()->setAlignment(Qt::AlignRight);
        chart->legend()->setLabelBrush(QBrush(ThemePalette::subtext0())); chart->setMargins(QMargins(0,0,0,0));
        
        pie = new QPieSeries(); pie->setHoleSize(0.70); chart->addSeries(pie);
        
        view = new DonutChartView(chart); view->setRenderHint(QPainter::Antialiasing);
        view->setBackgroundBrush(Qt::transparent); view->setStyleSheet("background: transparent; border: none;");
        view->setMinimumHeight(200); // Allow it to grow
        l->addWidget(view, 1);
        return card;
    };

    mainGridLayout->addWidget(createDonutCard(cpuChart_, cpuPie_, cpuView_, cpuTitle_, "CPU Usage"), 3, 0);
    mainGridLayout->addWidget(createDonutCard(ramChart_, ramPie_, ramView_, ramTitle_, "RAM Usage"), 3, 1);
    mainGridLayout->addWidget(createDonutCard(trafficChart_, trafficPie_, trafficView_, trafficTitle_, "Traffic Ratio"), 3, 2);
    mainGridLayout->setRowStretch(3, 2);
    
    cpuPie_->append("In Use", 0)->setBrush(ThemePalette::red());
    cpuPie_->append("Free", 100)->setBrush(ThemePalette::green());
    ramPie_->append("In Use", 0)->setBrush(ThemePalette::peach());
    ramPie_->append("Free", 100)->setBrush(ThemePalette::green());
    trafficPie_->append("Normal", 1)->setBrush(ThemePalette::green());
    trafficPie_->append("Attack", 0)->setBrush(ThemePalette::red());

    applyTheme(ThemeMode::Dark);
}

void DashboardWidget::setupAnalytics() {
    tabAnalytics_ = new QWidget();
    auto* layout = new QVBoxLayout(tabAnalytics_);

    // Title label
    auto* titleLabel = new QLabel("Infrastructure Health");
    titleLabel->setStyleSheet(QString("color: %1; font-size: 13px; font-weight: bold; background: transparent; padding: 5px;").arg(ThemePalette::text().name()));
    layout->addWidget(titleLabel);

    auto* topLayout = new QHBoxLayout();
    auto* healthGrp = new QGroupBox("Global SLO Health (Last points)");
    healthGrp->setStyleSheet(QString("color: %1; font-weight: bold;").arg(ThemePalette::text().name()));
    auto* hL = new QVBoxLayout(healthGrp); sloHealth_ = new AlertGridWidget(); hL->addWidget(sloHealth_);
    topLayout->addWidget(healthGrp, 1);
    auto* topGrp = new QGroupBox("Network Topology");
    topGrp->setStyleSheet(QString("color: %1; font-weight: bold;").arg(ThemePalette::text().name()));
    auto* tL = new QVBoxLayout(topGrp); topologyWidget_ = new NetworkTopologyWidget(); tL->addWidget(topologyWidget_);
    topLayout->addWidget(topGrp, 2);
    layout->addLayout(topLayout, 1);
    auto* midLayout = new QHBoxLayout();

    auto setupTable = [](QTableWidget*& t, const QStringList& headers, bool useProgress = false) {
        t = new QTableWidget(); t->setColumnCount(headers.size()); t->setHorizontalHeaderLabels(headers);
        t->verticalHeader()->setVisible(false); t->horizontalHeader()->setStretchLastSection(true);
        t->setEditTriggers(QAbstractItemView::NoEditTriggers); t->setSelectionBehavior(QAbstractItemView::SelectRows);
        t->setAlternatingRowColors(true);
        t->setShowGrid(false);
        t->setStyleSheet(ThemePalette::tableStyleSheet());
        if (useProgress) t->setItemDelegate(new ProgressDelegate(t));
    };

    setupTable(tableSources_, {"#", "IP Addr", "Packets"}, true); midLayout->addWidget(tableSources_, 1);
    setupTable(tableTargets_, {"#", "Target (IP:Port)", "Packets"}); midLayout->addWidget(tableTargets_, 1);
    
    auto* portsGrp = new QGroupBox("Top Active Ports");
    portsGrp->setStyleSheet(QString("color: %1; font-weight: bold;").arg(ThemePalette::text().name()));
    auto* pL = new QVBoxLayout(portsGrp);
    topPortsChart_ = new QChart(); topPortsChart_->setBackgroundBrush(Qt::transparent);
    topPortsChart_->legend()->setAlignment(Qt::AlignBottom); topPortsChart_->legend()->setLabelBrush(QBrush(ThemePalette::subtext0()));
    topPortsPie_ = new QPieSeries(); topPortsPie_->setHoleSize(0.4); topPortsChart_->addSeries(topPortsPie_);
    auto* pv = new QChartView(topPortsChart_); pv->setRenderHint(QPainter::Antialiasing); pL->addWidget(pv);
    midLayout->addWidget(portsGrp, 1);
    layout->addLayout(midLayout, 2);
    auto* botLayout = new QHBoxLayout();
    auto* bwGrp = new QGroupBox("Network Bandwidth"); bwGrp->setStyleSheet(QString("color: %1; font-weight: bold;").arg(ThemePalette::text().name()));
    auto* bL = new QVBoxLayout(bwGrp); bandwidthChart_ = new QChart(); bandwidthChart_->setBackgroundBrush(Qt::transparent);
    bandwidthChart_->legend()->hide(); bandwidthSeries_ = new QLineSeries(); bandwidthSeries_->setPen(QPen(ThemePalette::sapphire(), 2));
    bandwidthChart_->addSeries(bandwidthSeries_); bwTimeAxis_ = new QDateTimeAxis(); bwTimeAxis_->setFormat("HH:mm");
    bwAxis_ = new QValueAxis(); bwAxis_->setTitleText("Mbps");
    bandwidthChart_->addAxis(bwTimeAxis_, Qt::AlignBottom); bandwidthChart_->addAxis(bwAxis_, Qt::AlignLeft);
    bandwidthSeries_->attachAxis(bwTimeAxis_); bandwidthSeries_->attachAxis(bwAxis_);
    auto* bv = new QChartView(bandwidthChart_); bv->setRenderHint(QPainter::Antialiasing); bL->addWidget(bv);
    botLayout->addWidget(bwGrp, 1);
    auto* hmGrp = new QGroupBox("Port Activity Heatmap"); hmGrp->setStyleSheet(QString("color: %1; font-weight: bold;").arg(ThemePalette::text().name()));
    auto* hmL = new QVBoxLayout(hmGrp); heatmapWidget_ = new HeatmapWidget(); hmL->addWidget(heatmapWidget_);
    botLayout->addWidget(hmGrp, 1);
    auto* sizeGrp = new QGroupBox("Packet Size Distribution"); sizeGrp->setStyleSheet(QString("color: %1; font-weight: bold;").arg(ThemePalette::text().name()));
    auto* sL = new QVBoxLayout(sizeGrp); packetSizeChart_ = new QChart(); packetSizeChart_->setBackgroundBrush(Qt::transparent);
    packetSizeChart_->legend()->hide(); packetSizeSeries_ = new QBarSeries(); packetSizeSet_ = new QBarSet("Count");
    packetSizeSet_->setColor(ThemePalette::sapphire()); packetSizeSeries_->append(packetSizeSet_);
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
    
    QColor ppsColor = ThemePalette::sapphire();
    QColor tcpColor = ThemePalette::green();
    QColor udpColor = ThemePalette::yellow();
    QColor icmpColor = ThemePalette::red();
    QColor otherColor = ThemePalette::mauve();
    
    auto styleArea = [](QAreaSeries* s, const QColor& c) {
        if (!s) return;
        QColor fill = c; fill.setAlpha(60); 
        s->setBrush(fill);
        s->setPen(QPen(c, 1));
    };
    
    // Find areas - they are not stored as members for all, so we cast if needed or just use members
    // I added ppsArea, otherArea in setupDashboard local vars. 
    // I should have made them members to style them easily.
    // Let's find them in the chart series list.
    for (auto* s : ppsChart_->series()) {
        auto* area = qobject_cast<QAreaSeries*>(s);
        if (!area) continue;
        if (area->name() == "PPS") styleArea(area, ppsColor);
        else if (area->name() == "TCP") styleArea(area, tcpColor);
        else if (area->name() == "UDP") styleArea(area, udpColor);
        else if (area->name() == "ICMP") styleArea(area, icmpColor);
        else if (area->name() == "Other") styleArea(area, otherColor);
    }
    
    QPen dashPen(ThemePalette::peach());
    dashPen.setWidth(2);
    dashPen.setStyle(Qt::DashLine);
    attackConfidenceUpper_->setPen(dashPen);
    
    timeAxis_->setLabelsColor(ThemePalette::textSecondary());
    ppsAxis_->setLabelsColor(ThemePalette::textSecondary());
    confAxis_->setLabelsColor(ThemePalette::textSecondary());
}

void DashboardWidget::resetZoom() { ppsChart_->zoomReset(); userInteracting_ = false; }
QWidget* DashboardWidget::getTabAnalytics() const { return tabAnalytics_; }
QWidget* DashboardWidget::getTabSystem() const { return tabSystem_; }
void DashboardWidget::onUserInteracted() { userInteracting_ = true; autoScrollTimer_->start(10000); }

void DashboardWidget::onChartHover(const QPoint& pos) {
    if (!ppsChart_ || ppsSeries_->points().isEmpty()) return;

    QPointF value = ppsChart_->mapToValue(pos);
    qreal targetMs = value.x();

    // Find nearest point
    const auto& pts = ppsSeries_->points();
    auto it = std::lower_bound(pts.begin(), pts.end(), QPointF(targetMs, 0), [](const QPointF& a, const QPointF& b) {
        return a.x() < b.x();
    });

    if (it == pts.end()) it = std::prev(pts.end());
    if (it != pts.begin() && std::abs(std::prev(it)->x() - targetMs) < std::abs(it->x() - targetMs)) {
        it = std::prev(it);
    }

    int idx = std::distance(pts.begin(), it);
    if (idx < 0 || idx >= (int)pts.size()) return;

    QDateTime dt = QDateTime::fromMSecsSinceEpoch(pts[idx].x());
    
    QList<std::pair<QColor, QString>> items;
    double tcp = tcpUpper_->points()[idx].y() - zeroSeries_->points()[idx].y();
    double udp = udpUpper_->points()[idx].y() - tcpUpper_->points()[idx].y();
    double icmp = icmpUpper_->points()[idx].y() - udpUpper_->points()[idx].y();
    double other = otherSeries_->points()[idx].y();
    double total = pts[idx].y();

    items.append({ThemePalette::sapphire(), QString("Total: %1 pps").arg(total, 0, 'f', 1)});
    items.append({ThemePalette::green(), QString("TCP: %1 pps").arg(tcp, 0, 'f', 1)});
    items.append({ThemePalette::yellow(), QString("UDP: %1 pps").arg(udp, 0, 'f', 1)});
    items.append({ThemePalette::red(), QString("ICMP: %1 pps").arg(icmp, 0, 'f', 1)});
    items.append({ThemePalette::mauve(), QString("Other: %1 pps").arg(other, 0, 'f', 1)});
    
    double conf = attackConfidenceUpper_->points()[idx].y();
    QString footer = QString("Attack Confidence: %1%").arg(conf, 0, 'f', 1);

    chartTooltip_->updateContent(dt.toString("HH:mm:ss"), items, footer);
    
    // Position tooltip
    int tx = pos.x() + 15;
    int ty = pos.y() + 15;
    if (tx + chartTooltip_->width() > ppsChartView_->width()) tx = pos.x() - chartTooltip_->width() - 15;
    if (ty + chartTooltip_->height() > ppsChartView_->height()) ty = pos.y() - chartTooltip_->height() - 15;
    
    chartTooltip_->move(tx, ty);
    chartTooltip_->show();
}

void DashboardWidget::onChartLeave() {
    if (chartTooltip_) chartTooltip_->hide();
}

void DashboardWidget::addDataPoint(const DetectionResult& result) {
    QDateTime t = result.timestamp.isValid() ? result.timestamp : QDateTime::currentDateTime();
    qreal ms = t.toMSecsSinceEpoch();
    
    qint64 windowSizeMs = 120000; // 2 minute sliding window
    
    // Check if we need to initialize the time axis
    if (timeHistory_.empty()) {
        // Start range from current time to current time + window
        // This ensures the first point is on the far LEFT.
        timeAxis_->setRange(t, t.addMSecs(windowSizeMs));
        bwTimeAxis_->setRange(t, t.addMSecs(windowSizeMs));
    }

    ppsSeries_->append(ms, result.pps);
    double dt = result.flowDuration > 0 ? result.flowDuration : 1.0;
    
    double tcpVal = result.tcpPackets / dt;
    double udpVal = result.udpPackets / dt;
    double icmpVal = result.icmpPackets / dt;
    double otherVal = result.otherPackets / dt;
    
    double base = 0;
    zeroSeries_->append(ms, base);
    base += tcpVal;
    tcpUpper_->append(ms, base);
    base += udpVal;
    udpUpper_->append(ms, base);
    base += icmpVal;
    icmpUpper_->append(ms, base);
    
    otherSeries_->append(ms, otherVal);
    attackConfidenceUpper_->append(ms, result.confidence * 100.0);
    bandwidthSeries_->append(ms, (result.totalBytes * 8.0) / 1000000.0 / dt);
    
    timeHistory_.push_back(t);
    while (timeHistory_.size() > MAX_CHART_POINTS) {
        ppsSeries_->remove(0); 
        zeroSeries_->remove(0);
        tcpUpper_->remove(0);
        udpUpper_->remove(0);
        icmpUpper_->remove(0);
        otherSeries_->remove(0); 
        attackConfidenceUpper_->remove(0);
        bandwidthSeries_->remove(0); 
        timeHistory_.pop_front();
    }
    
    if (!userInteracting_ && !timeHistory_.empty()) {
        qint64 firstMs = timeHistory_.front().toMSecsSinceEpoch();
        qint64 lastMs = ms;
        
        // If we have less than a window of data, keep start at first point
        // If we have more, scroll the window
        qint64 startMs = firstMs;
        qint64 endMs = firstMs + windowSizeMs;
        
        if (lastMs > endMs) {
            endMs = lastMs;
            startMs = endMs - windowSizeMs;
        }
        
        timeAxis_->setRange(QDateTime::fromMSecsSinceEpoch(startMs), QDateTime::fromMSecsSinceEpoch(endMs));
        bwTimeAxis_->setRange(QDateTime::fromMSecsSinceEpoch(startMs), QDateTime::fromMSecsSinceEpoch(endMs));

        // Y Auto-scaling: Find max in visible range [startMs, endMs]
        double maxVisiblePps = 10.0;
        const auto& pts = ppsSeries_->points();
        for (const auto& p : pts) {
            if (p.x() >= (qreal)startMs && p.x() <= (qreal)endMs) {
                maxVisiblePps = std::max(maxVisiblePps, p.y());
            }
        }
        ppsAxis_->setRange(0, std::max(10.0, maxVisiblePps * 1.15));

        double maxVisibleBw = 1.0;
        const auto& bwPts = bandwidthSeries_->points();
        for (const auto& p : bwPts) {
            if (p.x() >= (qreal)startMs && p.x() <= (qreal)endMs) {
                maxVisibleBw = std::max(maxVisibleBw, p.y());
            }
        }
        bwAxis_->setRange(0, std::max(1.0, maxVisibleBw * 1.15));
    }
}

void DashboardWidget::updateSnapshot(float pps, uint64_t totalPackets, int currentLabel) {
    lblPps_->setText(QString("Current PPS: %1").arg(pps, 0, 'f', 1));
    lblTotalPackets_->setText(QString("Total Packets: %1").arg(totalPackets));
}

void DashboardWidget::updateConnectionStatus(bool connected) {
    collectorConnected_ = connected;
    if (connected) {
        lblCollector_->setText("Collector: Connected (Live)");
        lblCollector_->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold; background: transparent;").arg(ThemePalette::green().name()));
    } else {
        lblCollector_->setText("Collector: Disconnected");
        lblCollector_->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold; background: transparent;").arg(ThemePalette::red().name()));
    }
}

void DashboardWidget::loadHistory(const std::vector<DetectionResult>& events) {
    ppsSeries_->clear(); 
    zeroSeries_->clear();
    tcpUpper_->clear();
    udpUpper_->clear();
    icmpUpper_->clear();
    otherSeries_->clear(); 
    attackConfidenceUpper_->clear();
    bandwidthSeries_->clear(); 
    timeHistory_.clear(); totalNormal_ = 0; totalAttack_ = 0;
    for (auto& e : events) { if(e.label == 1) totalAttack_++; else totalNormal_++; addDataPoint(e); }
}

void DashboardWidget::updateSystemMetrics() { auto m = metricsCollector_.collect(); updateDonuts(m.cpuUsagePercent, m.ramUsagePercent); }

void DashboardWidget::updateDonuts(double cpu, double ram) {
    cpuPie_->slices()[0]->setValue(cpu); cpuPie_->slices()[1]->setValue(100.0 - cpu);
    if (cpuView_) cpuView_->setCenterText(QString("%1%").arg(cpu, 0, 'f', 1));
    
    ramPie_->slices()[0]->setValue(ram); ramPie_->slices()[1]->setValue(100.0 - ram);
    if (ramView_) ramView_->setCenterText(QString("%1%").arg(ram, 0, 'f', 1));
    
    uint64_t total = totalNormal_ + totalAttack_;
    trafficPie_->slices()[0]->setValue(totalNormal_); trafficPie_->slices()[1]->setValue(totalAttack_);
    double attPct = total > 0 ? (double)totalAttack_ / total * 100.0 : 0.0;
    if (trafficView_) trafficView_->setCenterText(QString("%1%").arg(attPct, 0, 'f', 1));
}

void DashboardWidget::updateRealtime(const DetectionResult& result, uint64_t totalPackets) {
    addDataPoint(result);
    if (result.label == 1) totalAttack_++; else totalNormal_++;
    
    lblTotalPackets_->setText(QString("Total Packets: %1").arg(totalPackets));
    lblPps_->setText(QString("Current PPS: %1").arg(result.pps, 0, 'f', 1));
    lblSources_->setText(QString("Sources: %1").arg(result.uniqueSourceCount));
    lblDropRate_->setText(QString("Drop Rate: %1 pps").arg(result.droppedPackets));
    lblDropRate_->setToolTip(QString("Total Dropped: %1 | Queue Size: %2").arg(result.droppedPackets).arg(result.queueSize));
    
    if (!result.blockedIps.empty()) {
        bpfCheckbox_->setToolTip(QString("Blocked IPs: %1 (e.g. %2)")
            .arg(result.blockedIps.size())
            .arg(QString::fromStdString(result.blockedIps.front())));
    } else {
        bpfCheckbox_->setToolTip("No IPs currently blocked by Firewall");
    }

    lblModel_->setText(QString("\u25CF ") + QString::fromStdString(result.modelName));
    lblModel_->setToolTip(QString("Inference Latency: %1 ms").arg(result.inferenceLatencyMs, 0, 'f', 2));
    
    lblProbability_->setText(QString("Probability: %1%").arg(result.confidence * 100, 0, 'f', 1));
    if (result.label == 1) {
        lblProbability_->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold; background: transparent;").arg(ThemePalette::red().name()));
        lblStatus_->setText("\u26A0 ATTACK DETECTED"); 
        lblStatus_->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold; background: transparent;").arg(ThemePalette::red().name()));
    } else {
        lblProbability_->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold; background: transparent;").arg(ThemePalette::green().name()));
        lblStatus_->setText("\u2714 Normal Traffic"); 
        lblStatus_->setStyleSheet(QString("color: %1; font-size: 12px; font-weight: bold; background: transparent;").arg(ThemePalette::green().name()));
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