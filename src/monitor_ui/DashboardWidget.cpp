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
    mainGridLayout->setContentsMargins(15, 15, 15, 15);
    mainGridLayout->setSpacing(15);

    // Helper to create a stylized card
    auto createCard = [](QWidget* parent = nullptr) {
        auto* card = new QFrame(parent);
        card->setStyleSheet("QFrame { background: #1e1e2e; border: 1px solid #313244; border-radius: 8px; }");
        return card;
    };

    // 1. Overview Bar (Top Row)
    auto* overviewCard = createCard();
    auto* overviewLayout = new QHBoxLayout(overviewCard);
    overviewLayout->setContentsMargins(20, 15, 20, 15);
    overviewLayout->setSpacing(20);
    
    auto createStatPanel = [](const QString& title, const QString& initValue, const QString& valColor = "#cdd6f4") {
        auto* panel = new QWidget();
        auto* l = new QVBoxLayout(panel);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(2);
        
        auto* tLbl = new QLabel(title);
        tLbl->setStyleSheet("color: #a6adc8; font-size: 11px; font-weight: bold; text-transform: uppercase; border: none; background: transparent;");
        
        auto* vLbl = new QLabel(initValue);
        vLbl->setStyleSheet(QString("color: %1; font-size: 18px; font-weight: bold; border: none; background: transparent;").arg(valColor));
        
        l->addWidget(tLbl);
        l->addWidget(vLbl);
        return std::make_pair(panel, vLbl);
    };

    auto [pColl, lColl] = createStatPanel("Collector", "Disconnected", "#f38ba8"); lblCollector_ = lColl;
    auto [pPkts, lPkts] = createStatPanel("Total Packets", "0"); lblTotalPackets_ = lPkts;
    auto [pPps, lPps]   = createStatPanel("Current PPS", "0.0"); lblPps_ = lPps;
    auto [pDrop, lDrop] = createStatPanel("Drop Rate", "0.0 pps", "#fab387"); lblDropRate_ = lDrop;
    auto [pSrc, lSrc]   = createStatPanel("Sources", "0"); lblSources_ = lSrc;
    auto [pFlow, lFlow] = createStatPanel("Flows", "0"); lblFlows_ = lFlow;
    
    bpfCheckbox_ = new QCheckBox("Auto-Block Top-IP");
    bpfCheckbox_->setStyleSheet("QCheckBox { color: #cdd6f4; font-size: 12px; font-weight: bold; border: none; background: transparent; }");
    connect(bpfCheckbox_, &QCheckBox::toggled, this, &DashboardWidget::bpfToggled);
    
    auto [pMod, lMod]   = createStatPanel("Model", "None", "#f5c2e7"); lblModel_ = lMod;
    auto [pProb, lProb] = createStatPanel("Attack Prob.", "0.0%", "#a6e3a1"); lblProbability_ = lProb;
    auto [pStat, lStat] = createStatPanel("Status", "✔ Normal", "#a6e3a1"); lblStatus_ = lStat;

    overviewLayout->addWidget(pColl);
    overviewLayout->addWidget(pPkts);
    overviewLayout->addWidget(pPps);
    overviewLayout->addWidget(pDrop);
    overviewLayout->addWidget(pSrc);
    overviewLayout->addWidget(pFlow);
    overviewLayout->addWidget(bpfCheckbox_);
    overviewLayout->addStretch();
    overviewLayout->addWidget(pMod);
    overviewLayout->addWidget(pProb);
    overviewLayout->addWidget(pStat);

    mainGridLayout->addWidget(overviewCard, 0, 0, 1, 3); // Spans across 3 columns

    // 2. Main Area Chart Card
    auto* chartCard = createCard();
    auto* chartCardLayout = new QVBoxLayout(chartCard);
    chartCardLayout->setContentsMargins(10, 10, 10, 10);
    chartCardLayout->setSpacing(5);

    auto* legendLayout = new QHBoxLayout();
    legendLayout->setContentsMargins(10, 0, 10, 0);
    legendLayout->setSpacing(15);
    
    auto* chartTitle = new QLabel("Traffic Rate & Attack Confidence");
    chartTitle->setStyleSheet("color: #cdd6f4; font-size: 14px; font-weight: bold; border: none; background: transparent;");
    legendLayout->addWidget(chartTitle);
    legendLayout->addStretch();

    auto createLayerCb = [&](const QString& text, const QColor& color, QCheckBox*& cb) {
        cb = new QCheckBox(text); cb->setChecked(true);
        cb->setStyleSheet(QString("QCheckBox { color: %1; font-weight: bold; font-size: 11px; border: none; background: transparent; }").arg(color.name()));
        legendLayout->addWidget(cb);
    };
    
    createLayerCb("PPS", QColor(116, 199, 236), cbPps_);
    createLayerCb("Other", QColor(166, 173, 200), cbOther_);
    createLayerCb("ICMP", QColor(243, 139, 168), cbIcmp_);
    createLayerCb("UDP", QColor(249, 226, 175), cbUdp_);
    createLayerCb("TCP", QColor(166, 227, 161), cbTcp_);
    createLayerCb("Confidence %", QColor(250, 179, 135), cbConf_);
    
    chartCardLayout->addLayout(legendLayout);

    ppsChart_ = new QChart();
    ppsChart_->setAnimationOptions(QChart::NoAnimation);
    ppsChart_->setBackgroundBrush(Qt::transparent);
    ppsChart_->legend()->hide();
    ppsChart_->setMargins(QMargins(0,0,0,0));
    
    ppsSeries_ = new QLineSeries(); 
    otherSeries_ = new QLineSeries();
    
    zeroSeries_ = new QLineSeries();
    tcpUpper_ = new QLineSeries();   tcpArea_ = new QAreaSeries(tcpUpper_, zeroSeries_);
    udpUpper_ = new QLineSeries();   udpArea_ = new QAreaSeries(udpUpper_, tcpUpper_);
    icmpUpper_ = new QLineSeries();  icmpArea_ = new QAreaSeries(icmpUpper_, udpUpper_);
    
    attackConfidenceUpper_ = new QLineSeries();
    attackConfidenceArea_ = new QAreaSeries(attackConfidenceUpper_, new QLineSeries());
    
    // Z-Order: Area charts at the bottom, lines on top
    ppsChart_->addSeries(attackConfidenceArea_);
    ppsChart_->addSeries(tcpArea_);
    ppsChart_->addSeries(udpArea_);
    ppsChart_->addSeries(icmpArea_);
    ppsChart_->addSeries(otherSeries_);
    ppsChart_->addSeries(ppsSeries_);
    
    timeAxis_ = new QDateTimeAxis(); timeAxis_->setFormat("HH:mm:ss");
    timeAxis_->setGridLineColor(QColor(49, 50, 68)); timeAxis_->setLinePenColor(Qt::transparent);
    ppsChart_->addAxis(timeAxis_, Qt::AlignBottom);
    
    ppsAxis_ = new QValueAxis(); ppsAxis_->setGridLineColor(QColor(49, 50, 68)); ppsAxis_->setLinePenColor(Qt::transparent);
    ppsChart_->addAxis(ppsAxis_, Qt::AlignLeft);
    
    confAxis_ = new QValueAxis(); confAxis_->setRange(0, 100); confAxis_->setGridLineVisible(false); confAxis_->setLinePenColor(Qt::transparent);
    ppsChart_->addAxis(confAxis_, Qt::AlignRight);
    
    ppsSeries_->attachAxis(timeAxis_); ppsSeries_->attachAxis(ppsAxis_);
    tcpArea_->attachAxis(timeAxis_); tcpArea_->attachAxis(ppsAxis_);
    udpArea_->attachAxis(timeAxis_); udpArea_->attachAxis(ppsAxis_);
    icmpArea_->attachAxis(timeAxis_); icmpArea_->attachAxis(ppsAxis_);
    otherSeries_->attachAxis(timeAxis_); otherSeries_->attachAxis(ppsAxis_);
    attackConfidenceArea_->attachAxis(timeAxis_); attackConfidenceArea_->attachAxis(confAxis_);
    
    connect(cbPps_, &QCheckBox::toggled, ppsSeries_, &QLineSeries::setVisible);
    connect(cbTcp_, &QCheckBox::toggled, tcpArea_, &QAreaSeries::setVisible);
    connect(cbUdp_, &QCheckBox::toggled, udpArea_, &QAreaSeries::setVisible);
    connect(cbIcmp_, &QCheckBox::toggled, icmpArea_, &QAreaSeries::setVisible);
    connect(cbOther_, &QCheckBox::toggled, otherSeries_, &QLineSeries::setVisible);
    connect(cbConf_, &QCheckBox::toggled, attackConfidenceArea_, &QAreaSeries::setVisible);
    
    ppsChartView_ = new InteractiveChartView(ppsChart_, chartCard);
    ppsChartView_->setRenderHint(QPainter::Antialiasing);
    ppsChartView_->setStyleSheet("background: transparent; border: none;");
    connect(ppsChartView_, &InteractiveChartView::userInteracted, this, &DashboardWidget::onUserInteracted);
    connect(ppsChartView_, &InteractiveChartView::mouseMoved, this, &DashboardWidget::onChartHover);
    connect(ppsChartView_, &InteractiveChartView::mouseLeft, this, &DashboardWidget::onChartLeave);
    
    chartTooltip_ = new SmartTooltip(ppsChartView_);
    
    chartCardLayout->addWidget(ppsChartView_, 1);
    mainGridLayout->addWidget(chartCard, 1, 0, 1, 3); // Spans across 3 columns
    mainGridLayout->setRowStretch(1, 3); // Give the chart more vertical space

    // 3. Donuts Bottom (3 Columns)
    auto createDonutCard = [&](QChart*& chart, QPieSeries*& pie, DonutChartView*& view, QLabel*& titleLbl, const QString& titleText) {
        auto* card = createCard();
        auto* l = new QVBoxLayout(card);
        l->setContentsMargins(15, 15, 15, 15);
        
        titleLbl = new QLabel(titleText);
        titleLbl->setAlignment(Qt::AlignLeft);
        titleLbl->setStyleSheet("color: #cdd6f4; font-weight: bold; font-size: 14px; border: none; background: transparent;");
        l->addWidget(titleLbl);
        
        chart = new QChart(); chart->setAnimationOptions(QChart::NoAnimation);
        chart->setBackgroundBrush(Qt::transparent); chart->legend()->setAlignment(Qt::AlignRight);
        chart->legend()->setLabelBrush(QBrush(QColor("#a6adc8"))); chart->setMargins(QMargins(0,0,0,0));
        
        pie = new QPieSeries(); pie->setHoleSize(0.70); chart->addSeries(pie);
        
        view = new DonutChartView(chart); view->setRenderHint(QPainter::Antialiasing);
        view->setBackgroundBrush(Qt::transparent); view->setStyleSheet("background: transparent; border: none;");
        view->setMinimumHeight(200); // Allow it to grow
        l->addWidget(view, 1);
        return card;
    };

    mainGridLayout->addWidget(createDonutCard(cpuChart_, cpuPie_, cpuView_, cpuTitle_, "CPU Usage"), 2, 0);
    mainGridLayout->addWidget(createDonutCard(ramChart_, ramPie_, ramView_, ramTitle_, "RAM Usage"), 2, 1);
    mainGridLayout->addWidget(createDonutCard(trafficChart_, trafficPie_, trafficView_, trafficTitle_, "Traffic Ratio"), 2, 2);
    mainGridLayout->setRowStretch(2, 2);
    
    cpuPie_->append("In Use", 0)->setBrush(QColor("#f38ba8"));
    cpuPie_->append("Free", 100)->setBrush(QColor("#a6e3a1"));
    ramPie_->append("In Use", 0)->setBrush(QColor("#fab387"));
    ramPie_->append("Free", 100)->setBrush(QColor("#a6e3a1"));
    trafficPie_->append("Normal", 1)->setBrush(QColor("#a6e3a1"));
    trafficPie_->append("Attack", 0)->setBrush(QColor("#f38ba8"));

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

    auto setupTable = [](QTableWidget*& t, const QStringList& headers, bool useProgress = false) {
        t = new QTableWidget(); t->setColumnCount(headers.size()); t->setHorizontalHeaderLabels(headers);
        t->verticalHeader()->setVisible(false); t->horizontalHeader()->setStretchLastSection(true);
        t->setEditTriggers(QAbstractItemView::NoEditTriggers); t->setSelectionBehavior(QAbstractItemView::SelectRows);
        t->setAlternatingRowColors(true);
        t->setShowGrid(false);
        
        QString qss = R"(
            QTableWidget {
                background-color: #1e1e2e;
                alternate-background-color: #242437;
                color: #cdd6f4;
                gridline-color: transparent;
                border: 1px solid #313244;
                border-radius: 8px;
                padding: 5px;
            }
            QHeaderView::section {
                background-color: #181825;
                color: #a6adc8;
                font-weight: bold;
                padding: 8px;
                border: none;
                text-transform: uppercase;
                font-size: 10px;
            }
            QTableWidget::item {
                padding: 10px;
                border-bottom: 1px solid #313244;
            }
            QTableWidget::item:selected {
                background-color: #45475a;
                color: #f5e0dc;
            }
        )";
        t->setStyleSheet(qss);
        if (useProgress) t->setItemDelegate(new ProgressDelegate(t));
    };

    setupTable(tableSources_, {"#", "IP Addr", "Packets"}, true); midLayout->addWidget(tableSources_, 1);
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
    
    QColor ppsColor = QColor(100, 200, 255);
    QColor tcpColor = QColor(100, 220, 100);
    QColor udpColor = QColor(255, 200, 100);
    QColor icmpColor = QColor(255, 100, 100);
    QColor otherColor = QColor(160, 160, 160);
    
    ppsSeries_->setPen(QPen(ppsColor, 2));
    
    auto styleArea = [](QAreaSeries* s, const QColor& c) {
        QColor fill = c; fill.setAlpha(80); // Semi-transparent
        s->setBrush(fill);
        s->setPen(QPen(c, 1));
    };
    
    styleArea(tcpArea_, tcpColor);
    styleArea(udpArea_, udpColor);
    styleArea(icmpArea_, icmpColor);
    otherSeries_->setPen(QPen(otherColor, 1.5));
    
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

    items.append({QColor(116, 199, 236), QString("Total: %1 pps").arg(total, 0, 'f', 1)});
    items.append({QColor(166, 227, 161), QString("TCP: %1 pps").arg(tcp, 0, 'f', 1)});
    items.append({QColor(249, 226, 175), QString("UDP: %1 pps").arg(udp, 0, 'f', 1)});
    items.append({QColor(243, 139, 168), QString("ICMP: %1 pps").arg(icmp, 0, 'f', 1)});
    items.append({QColor(166, 173, 200), QString("Other: %1 pps").arg(other, 0, 'f', 1)});
    
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
    ppsSeries_->append(ms, result.pps);
    double dt = result.flowDuration > 0 ? result.flowDuration : 1.0;
    
    double tcpVal = result.tcpPackets / dt;
    double udpVal = result.udpPackets / dt;
    double icmpVal = result.icmpPackets / dt;
    
    double base = 0;
    zeroSeries_->append(ms, base);
    base += tcpVal;
    tcpUpper_->append(ms, base);
    base += udpVal;
    udpUpper_->append(ms, base);
    base += icmpVal;
    icmpUpper_->append(ms, base);
    
    otherSeries_->append(ms, result.otherPackets / dt);
    
    attackConfidenceUpper_->append(ms, result.confidence * 100.0);
    qobject_cast<QLineSeries*>(attackConfidenceArea_->lowerSeries())->append(ms, 0);
    bandwidthSeries_->append(ms, (result.totalBytes * 8.0) / 1000000.0 / dt);
    
    timeHistory_.push_back(t);
    while (timeHistory_.size() > 300) {
        ppsSeries_->remove(0); 
        zeroSeries_->remove(0);
        tcpUpper_->remove(0);
        udpUpper_->remove(0);
        icmpUpper_->remove(0);
        otherSeries_->remove(0); 
        attackConfidenceUpper_->remove(0);
        qobject_cast<QLineSeries*>(attackConfidenceArea_->lowerSeries())->remove(0);
        bandwidthSeries_->remove(0); 
        timeHistory_.pop_front();
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
    lblPps_->setText(QString::number(pps, 'f', 1));
    lblTotalPackets_->setText(QString::number(totalPackets));
}

void DashboardWidget::updateConnectionStatus(bool connected) {
    if (connected) { lblCollector_->setText("Connected (Live)"); lblCollector_->setStyleSheet("color: #66ff66; font-size: 18px; font-weight: bold; border: none; background: transparent;"); }
    else { lblCollector_->setText("Disconnected"); lblCollector_->setStyleSheet("color: #ff6666; font-size: 18px; font-weight: bold; border: none; background: transparent;"); }
}

void DashboardWidget::loadHistory(const std::vector<DetectionResult>& events) {
    ppsSeries_->clear(); 
    zeroSeries_->clear();
    tcpUpper_->clear();
    udpUpper_->clear();
    icmpUpper_->clear();
    otherSeries_->clear(); 
    attackConfidenceUpper_->clear();
    qobject_cast<QLineSeries*>(attackConfidenceArea_->lowerSeries())->clear();
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
    
    lblTotalPackets_->setText(QString::number(totalPackets));
    lblPps_->setText(QString::number(result.pps, 'f', 1));
    lblSources_->setText(QString::number(result.uniqueSourceCount));
    lblFlows_->setText(QString::number(result.activeFlowsCount));
    lblDropRate_->setText(QString("%1 pps").arg(result.droppedPackets));
    lblDropRate_->setToolTip(QString("Total Dropped: %1 | Queue Size: %2").arg(result.droppedPackets).arg(result.queueSize));
    
    lblModel_->setText("🧠 " + QString::fromStdString(result.modelName));
    lblModel_->setToolTip(QString("Inference Latency: %1 ms").arg(result.inferenceLatencyMs, 0, 'f', 2));
    
    lblProbability_->setText(QString("%1%").arg(result.confidence * 100, 0, 'f', 1));
    if (result.label == 1) {
        lblProbability_->setStyleSheet("color: #ff6666; font-size: 18px; font-weight: bold; border: none; background: transparent;");
        lblStatus_->setText("⚠ ATTACK DETECTED"); 
        lblStatus_->setStyleSheet("color: #ff6666; font-size: 18px; font-weight: bold; border: none; background: transparent;");
    } else {
        lblProbability_->setStyleSheet("color: #66ff66; font-size: 18px; font-weight: bold; border: none; background: transparent;");
        lblStatus_->setText("✔ Normal Traffic"); 
        lblStatus_->setStyleSheet("color: #66ff66; font-size: 18px; font-weight: bold; border: none; background: transparent;");
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