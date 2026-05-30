/**
 * @file DashboardWidget.cpp
 * @brief Главная панель мониторинга (реализация).
 *
 * Назначение: Реализация методов визуализации трафика и управления состоянием интерфейса.
 * Входные данные: Статистические показатели в формате DetectionResult.
 * Результаты: Динамическое обновление графиков, таблиц и индикаторов.
 * Метод решения: Использование сигнально-слотовой архитектуры Qt для обработки потока данных.
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

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
#include <QFontMetrics>
#include <set>

// ====== ColorCheckBox ======
ColorCheckBox::ColorCheckBox(const QString& label, const QColor& color, QWidget* parent)
    : QWidget(parent), label_(label), color_(color) {
    setCursor(Qt::PointingHandCursor);
    QFontMetrics fm(font());
    setFixedHeight(22);
    setMinimumWidth(fm.horizontalAdvance(label) + 26);
}

void ColorCheckBox::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    // Draw color square
    QRect sq(0, (height()-12)/2, 12, 12);
    if (checked_) {
        p.setBrush(color_);
        p.setPen(QPen(color_.darker(120), 1));
    } else {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(ThemePalette::surface2(), 1.5));
    }
    p.drawRoundedRect(sq, 2, 2);
    // Draw label
    p.setPen(checked_ ? color_ : ThemePalette::overlay0());
    QFont f = p.font(); f.setPointSize(10); f.setBold(checked_); p.setFont(f);
    p.drawText(QRect(17, 0, width()-17, height()), Qt::AlignVCenter, label_);
}

void ColorCheckBox::mousePressEvent(QMouseEvent*) {
    checked_ = !checked_;
    update();
    emit toggled(checked_);
}

// ====== HeatmapWidget ======
HeatmapWidget::HeatmapWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(200, 150);
}

void HeatmapWidget::addData(const QDateTime& time, const std::map<uint16_t, uint64_t>& portData) {
    // Sliding window — храним последние 5 минут
    QDateTime cutoff = time.addSecs(-300);
    while (!points_.empty() && points_.front().t < cutoff) points_.pop_front();

    uint64_t maxVal = 1;
    for (auto& [p, c] : portData) maxVal = std::max(maxVal, c);
    for (auto& [p, c] : portData) points_.push_back({time, p, (double)c / maxVal});

    if (!points_.empty()) { 
        maxTime_ = QDateTime::currentDateTime();
        minTime_ = maxTime_.addSecs(-300); // Fixed 5 minute window
    }
    update();
}

void HeatmapWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), ThemePalette::crust());

    const int marginL = 40, marginR = 8, marginT = 6, marginB = 22;
    QRect plotRect(marginL, marginT, width()-marginL-marginR, height()-marginT-marginB);

    if (points_.empty()) {
        p.setPen(ThemePalette::textSecondary());
        p.drawText(plotRect, Qt::AlignCenter, "No heatmap data");
        return;
    }

    qint64 tRange = 300000; // 5 minutes in ms
    qint64 minMs = minTime_.toMSecsSinceEpoch();

    // Collect unique ports for Y axis
    std::set<uint16_t> portSet;
    for (auto& pt : points_) portSet.insert(pt.port);
    std::vector<uint16_t> ports(portSet.rbegin(), portSet.rend()); // desc
    if (ports.empty()) return;

    int nPorts = std::min((int)ports.size(), 20);
    double cellH = (double)plotRect.height() / nPorts;

    for (auto& pt : points_) {
        qint64 dt = pt.t.toMSecsSinceEpoch() - minMs;
        if (dt < 0) continue; // Skip points outside current window

        int x = plotRect.left() + (int)((double)dt / tRange * plotRect.width());
        if (x > plotRect.right()) continue; 

        auto it = std::find(ports.begin(), ports.begin()+nPorts, pt.port);
        if (it == ports.begin()+nPorts) continue;
        int portIdx = (int)(it - ports.begin());
        int y = plotRect.top() + (int)(portIdx * cellH);

        // Interpolate colour: blue → yellow → red
        QColor c;
        if (pt.intensity < 0.5)
            c = QColor::fromHsvF(0.6 - pt.intensity*0.4, 0.8, 0.9);
        else
            c = QColor::fromHsvF(0.1 - (pt.intensity-0.5)*0.2, 0.9, 1.0);
        c.setAlpha(180);

        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(x-3, y+2, 7, std::max(3,(int)cellH-4), 2, 2);
    }

    // Y axis labels (port numbers)
    p.setPen(ThemePalette::subtext1());
    QFont f = p.font(); f.setPointSize(8); p.setFont(f);
    for (int i = 0; i < nPorts; i++) {
        int y = plotRect.top() + (int)(i * cellH + cellH/2);
        p.drawText(QRect(0, y-8, marginL-4, 16), Qt::AlignRight|Qt::AlignVCenter,
                   QString::number(ports[i]));
    }

    // X axis — time labels
    p.setPen(ThemePalette::subtext1());
    p.drawText(QRect(plotRect.left(), plotRect.bottom()+4, 60, 16),
               Qt::AlignLeft, minTime_.toString("HH:mm:ss"));
    p.drawText(QRect(plotRect.right()-60, plotRect.bottom()+4, 60, 16),
               Qt::AlignRight, maxTime_.toString("HH:mm:ss"));
}

// ====== NetworkTopologyWidget ======
NetworkTopologyWidget::NetworkTopologyWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(300, 200);
}

void NetworkTopologyWidget::updateTopology(const std::vector<std::pair<std::string, uint64_t>>& targets, double ingressMbps) {
    // Aggregate by IP (strip port)
    std::map<std::string, uint64_t> aggregated;
    for (const auto& t : targets) {
        QString fullAddr = QString::fromStdString(t.first);
        std::string ipOnly = fullAddr.section(':', 0, 0).toStdString();
        aggregated[ipOnly] += t.second;
    }

    // Convert back to vector and sort by packet count
    targets_.clear();
    for (auto const& [ip, count] : aggregated) {
        targets_.push_back({ip, count});
    }
    std::sort(targets_.begin(), targets_.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    if(targets_.size() > 5) targets_.resize(5);
    ingressMbps_ = ingressMbps; update();
}

void NetworkTopologyWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), ThemePalette::crust());

    QFont titleFont = p.font(); titleFont.setPointSize(10); titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(ThemePalette::textPrimary());
    p.drawText(QRect(8, 4, width()-16, 22), Qt::AlignCenter,
               QString("Ingress: %1 Mbps").arg(ingressMbps_, 0, 'f', 2));

    if (targets_.empty()) return;

    int monX = 40;
    int monY = (height() + 28) / 2;
    int tarX = width() - 110;  // Leave space for IP labels on the right
    int n = (int)targets_.size();
    int usableH = height() - 36;
    int tarYStep = n > 0 ? usableH / n : usableH;

    // Monitor node
    p.setBrush(ThemePalette::sapphire());
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPoint(monX, monY), 14, 14);

    QFont nodeFont = p.font(); nodeFont.setPointSize(8); nodeFont.setBold(true);
    p.setFont(nodeFont);
    p.setPen(ThemePalette::text());
    p.drawText(QRect(monX-36, monY+18, 72, 16), Qt::AlignCenter, "Monitor");

    QFont smallFont = p.font(); smallFont.setPointSize(8); smallFont.setBold(false);

    for (int i = 0; i < n; i++) {
        int ty = 28 + i * tarYStep + tarYStep/2;

        // Line
        p.setPen(QPen(ThemePalette::surface1(), 1.5));
        p.drawLine(monX+14, monY, tarX-10, ty);

        // Packet count label mid-line
        int mx = (monX + tarX) / 2;
        int my = (monY + ty) / 2;
        p.setFont(smallFont);
        p.setPen(ThemePalette::subtext0());
        p.drawText(QRect(mx-30, my-10, 60, 16), Qt::AlignCenter,
                   QString("%1 p").arg(targets_[i].second));

        // Target node
        p.setPen(Qt::NoPen);
        p.setBrush(ThemePalette::danger());
        p.drawEllipse(QPoint(tarX, ty), 8, 8);

        // IP label — right of target circle (strip port if exists)
        p.setFont(nodeFont);
        p.setPen(ThemePalette::textPrimary());
        QString fullAddr = QString::fromStdString(targets_[i].first);
        QString ipOnly = fullAddr.section(':', 0, 0); 
        p.drawText(QRect(tarX + 12, ty - 10, 100, 20), Qt::AlignLeft | Qt::AlignVCenter, ipOnly);
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

// Clamp Y axis of all QValueAxis to >= 0
static void clampAxesAboveZero(QChart* chart) {
    for (auto* ax : chart->axes(Qt::Vertical)) {
        auto* vax = qobject_cast<QValueAxis*>(ax);
        if (vax && vax->min() < 0.0) {
            qreal span = vax->max() - vax->min();
            vax->setRange(0.0, std::max(1.0, span));
        }
    }
}

void InteractiveChartView::wheelEvent(QWheelEvent* event) {
    emit userInteracted();
    qreal factor = event->angleDelta().y() > 0 ? 1.1 : 0.9;
    
    QRectF r = chart()->plotArea();
    qreal newWidth = r.width() / factor;
    qreal centerX = r.center().x();
    r.setLeft(centerX - newWidth / 2.0);
    r.setRight(centerX + newWidth / 2.0);
    chart()->zoomIn(r);
    
    clampAxesAboveZero(chart());
    event->accept();
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
        chart()->scroll(-delta.x(), delta.y());
        clampAxesAboveZero(chart());
        lastMousePos_ = event->position();
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
    metricsTimer_->start(1000);
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
        card->setProperty("cssClass", "dashboardCard");
        return card;
    };

    // Title label
    auto* titleLabel = new QLabel("Overview (Global State)");
    titleLabel->setProperty("cssClass", "overviewTitle");
    mainGridLayout->addWidget(titleLabel, 0, 0, 1, 3);

    // 1. Overview Status Bar (single horizontal row)
    auto* overviewBar = new QWidget();
    overviewBar->setFixedHeight(32);
    overviewBar->setStyleSheet("background: transparent;");
    auto* overviewLayout = new QHBoxLayout(overviewBar);
    overviewLayout->setContentsMargins(5, 0, 5, 0);
    overviewLayout->setSpacing(15);

    auto createStatusLabel = [](const QString& text, const QString& cssClass) {
        auto* lbl = new QLabel(text);
        lbl->setProperty("cssClass", cssClass);
        return lbl;
    };

    lblCollector_ = createStatusLabel("Collector: Disconnected", "statusLabelRed");
    lblTotalPackets_ = createStatusLabel("Total Packets: 0", "statusLabelText");
    lblPps_ = createStatusLabel("Current PPS: 0.0", "statusLabelText");
    lblDropRate_ = createStatusLabel("Drop Rate: 0.0 pps", "statusLabelPeach");
    lblSources_ = createStatusLabel("Sources: 0", "statusLabelText");
    
    bpfCheckbox_ = new QCheckBox("Auto-Block Top-IP (Firewall)");
    bpfCheckbox_->setProperty("cssClass", "dashboardCheckbox");
    connect(bpfCheckbox_, &QCheckBox::toggled, this, &DashboardWidget::bpfToggled);

    auto* pcapCheckbox = new QCheckBox("Save PCAP Dumps");
    pcapCheckbox->setProperty("cssClass", "dashboardCheckbox");
    connect(pcapCheckbox, &QCheckBox::toggled, this, &DashboardWidget::pcapToggled);

    lblModel_ = createStatusLabel("\u25CF rf_model.onnx", "statusLabelText");
    lblProbability_ = createStatusLabel("Probability: 0.0%", "statusLabelGreen");
    lblStatus_ = createStatusLabel("\u2714 Normal Traffic", "statusLabelGreen");

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
    chartTitle->setProperty("cssClass", "chartTitle");
    legendLayout->addWidget(chartTitle);
    legendLayout->addStretch();

    auto createLayerCb = [&](const QString& text, const QColor& color, ColorCheckBox*& cb) {
        cb = new ColorCheckBox(text, color);
        legendLayout->addWidget(cb);
    };
    
    createLayerCb("PPS",        ThemePalette::sapphire(), cbPps_);
    createLayerCb("Other",      ThemePalette::mauve(),    cbOther_);
    createLayerCb("ICMP",       ThemePalette::red(),      cbIcmp_);
    createLayerCb("UDP",        ThemePalette::yellow(),   cbUdp_);
    createLayerCb("TCP",        ThemePalette::green(),    cbTcp_);
    createLayerCb("Confidence", ThemePalette::peach(),    cbConf_);
    
    resetBtn_ = new QPushButton("Reset Zoom");
    resetBtn_->setToolTip("Сбросить масштаб графика до исходного (двойной клик по графику)");
    legendLayout->addWidget(resetBtn_);

    chartCardLayout->addLayout(legendLayout);

    ppsChart_ = new QChart();
    ppsChart_->setAnimationOptions(QChart::NoAnimation);
    ppsChart_->setBackgroundBrush(Qt::transparent);
    ppsChart_->legend()->hide();
    ppsChart_->setMargins(QMargins(0,0,0,0));
    
    zeroSeries_ = new QLineSeries();
    zeroSeries_->setPen(QPen(Qt::transparent));

    // Настраиваем стили area-серий: полупрозрачная заливка + линия под цвет
    auto makeArea = [](QLineSeries* upper, QLineSeries* lower,
                       const QString& name, const QColor& c) -> QAreaSeries* {
        auto* area = new QAreaSeries(upper, lower);
        area->setName(name);
        QColor fill = c; fill.setAlpha(80);
        area->setBrush(fill);
        QPen pen(c, 2); area->setPen(pen);
        upper->setPen(QPen(c, 2));
        return area;
    };

    ppsSeries_ = new QLineSeries();
    ppsArea_  = makeArea(ppsSeries_, zeroSeries_, "PPS", ThemePalette::sapphire());

    otherSeries_ = new QLineSeries();
    otherArea_   = makeArea(otherSeries_, zeroSeries_, "Other", ThemePalette::mauve());

    tcpUpper_  = new QLineSeries();
    udpUpper_  = new QLineSeries();
    icmpUpper_ = new QLineSeries();
    tcpArea_   = makeArea(tcpUpper_,  zeroSeries_, "TCP",  ThemePalette::green());
    udpArea_   = makeArea(udpUpper_,  tcpUpper_,   "UDP",  ThemePalette::yellow());
    icmpArea_  = makeArea(icmpUpper_, udpUpper_,   "ICMP", ThemePalette::red());

    attackConfidenceUpper_ = new QLineSeries();
    attackConfidenceUpper_->setName("Attack Confidence");
    QPen dashPen(ThemePalette::peach());
    dashPen.setWidth(2.5);
    dashPen.setStyle(Qt::DashLine);
    attackConfidenceUpper_->setPen(dashPen);

    // Z-Order: Areas at bottom, confidence line on top
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
    
    connect(cbPps_,   &ColorCheckBox::toggled, ppsArea_,               &QAreaSeries::setVisible);
    connect(cbTcp_,   &ColorCheckBox::toggled, tcpArea_,               &QAreaSeries::setVisible);
    connect(cbUdp_,   &ColorCheckBox::toggled, udpArea_,               &QAreaSeries::setVisible);
    connect(cbIcmp_,  &ColorCheckBox::toggled, icmpArea_,              &QAreaSeries::setVisible);
    connect(cbOther_, &ColorCheckBox::toggled, otherArea_,             &QAreaSeries::setVisible);
    connect(cbConf_,  &ColorCheckBox::toggled, attackConfidenceUpper_, &QLineSeries::setVisible);
    connect(resetBtn_, &QPushButton::clicked,  this, &DashboardWidget::resetZoom);
    
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
        titleLbl->setProperty("cssClass", "donutTitle");
        l->addWidget(titleLbl);
        
        chart = new QChart(); chart->setAnimationOptions(QChart::NoAnimation);
        chart->setBackgroundBrush(Qt::transparent); chart->legend()->setAlignment(Qt::AlignRight);
        chart->legend()->setLabelBrush(QBrush(ThemePalette::textPrimary())); chart->setMargins(QMargins(0,0,0,0));
        
        pie = new QPieSeries(); pie->setHoleSize(0.70); chart->addSeries(pie);
        
        view = new DonutChartView(chart); view->setRenderHint(QPainter::Antialiasing);
        view->setBackgroundBrush(Qt::transparent); view->setStyleSheet("background: transparent; border: none;");
        view->setMinimumHeight(200); // Allow it to grow
        l->addWidget(view, 1);
        return card;
    };

    mainGridLayout->addWidget(createDonutCard(cpuChart_,     cpuPie_,     cpuView_,     cpuTitle_,     "CPU Usage"),     3, 0);
    mainGridLayout->addWidget(createDonutCard(ramChart_,     ramPie_,     ramView_,     ramTitle_,     "RAM Usage"),     3, 1);
    mainGridLayout->addWidget(createDonutCard(trafficChart_, trafficPie_, trafficView_, trafficTitle_, "Traffic Ratio"), 3, 2);
    mainGridLayout->setRowStretch(3, 2);
    
    // Donut — holeSize 0.55 чтобы кольцо было шире
    cpuPie_->setHoleSize(0.55);     cpuPie_->setPieSize(0.85);
    ramPie_->setHoleSize(0.55);     ramPie_->setPieSize(0.85);
    trafficPie_->setHoleSize(0.55); trafficPie_->setPieSize(0.85);
    
    cpuPie_->append("In Use", 0)->setBrush(ThemePalette::red());
    cpuPie_->append("Free", 100)->setBrush(ThemePalette::surface0());
    ramPie_->append("In Use", 0)->setBrush(ThemePalette::peach());
    ramPie_->append("Free", 100)->setBrush(ThemePalette::surface0());
    trafficPie_->append("Normal", 1)->setBrush(ThemePalette::green());
    trafficPie_->append("Attack", 0)->setBrush(ThemePalette::red());

    applyTheme(ThemeMode::Dark);
}

void DashboardWidget::setupAnalytics() {
    tabAnalytics_ = new QWidget();
    auto* layout = new QVBoxLayout(tabAnalytics_);

    // Title label
    auto* titleLabel = new QLabel("Infrastructure Health");
    titleLabel->setProperty("cssClass", "overviewTitle");
    layout->addWidget(titleLabel);

    auto* topLayout = new QHBoxLayout();
    auto* healthGrp = new QGroupBox("Global SLO Health (Last points)");
    healthGrp->setToolTip("История доступности сервиса (SLO). Зеленый - норма, Красный - аномалия/атака");
    auto* hL = new QVBoxLayout(healthGrp); sloHealth_ = new AlertGridWidget(); hL->addWidget(sloHealth_);
    topLayout->addWidget(healthGrp, 1);
    auto* topGrp = new QGroupBox("Network Topology");
    topGrp->setToolTip("Визуализация целевых IP-адресов и нагрузки на них");
    auto* tL = new QVBoxLayout(topGrp); topologyWidget_ = new NetworkTopologyWidget(); tL->addWidget(topologyWidget_);
    topLayout->addWidget(topGrp, 2);
    layout->addLayout(topLayout, 1);
    auto* midLayout = new QHBoxLayout();

    auto setupTable = [](QTableWidget*& t, const QStringList& headers, bool useProgress = false) {
        t = new QTableWidget();
        t->setColumnCount(headers.size());
        t->setHorizontalHeaderLabels(headers);
        t->verticalHeader()->setVisible(false);
        
        // Auto-resize columns to contents
        t->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        t->horizontalHeader()->setStretchLastSection(true);
        
        t->setEditTriggers(QAbstractItemView::NoEditTriggers);
        t->setSelectionBehavior(QAbstractItemView::SelectRows);
        t->setSelectionMode(QAbstractItemView::SingleSelection);
        t->setAlternatingRowColors(true);
        t->setShowGrid(false);
        t->setSortingEnabled(true);
        
        if (useProgress) {
            t->setItemDelegate(new ProgressDelegate(t));
        }
    };

    setupTable(tableSources_, {"#", "IP Addr", "Packets"}, true);
    midLayout->addWidget(tableSources_, 1);
    setupTable(tableTargets_, {"#", "Target (IP:Port)", "Packets"});
    midLayout->addWidget(tableTargets_, 1);
    
    auto* portsGrp = new QGroupBox("Top Active Ports");
    portsGrp->setToolTip("Распределение трафика по портам (Pie Chart)");
    auto* pL = new QVBoxLayout(portsGrp);
    topPortsChart_ = new QChart(); topPortsChart_->setBackgroundBrush(Qt::transparent);
    topPortsChart_->legend()->setAlignment(Qt::AlignBottom);
    topPortsChart_->legend()->setLabelBrush(QBrush(ThemePalette::textPrimary()));
    topPortsPie_ = new QPieSeries(); topPortsPie_->setHoleSize(0.4);
    topPortsChart_->addSeries(topPortsPie_);
    auto* pv = new QChartView(topPortsChart_);
    pv->setRenderHint(QPainter::Antialiasing);
    pv->setStyleSheet("background: transparent; border: none;");
    pL->addWidget(pv);
    midLayout->addWidget(portsGrp, 1);
    layout->addLayout(midLayout, 2);

    auto* botLayout = new QHBoxLayout();

    // Network Bandwidth — с area-заливкой под линию (п.7)
    auto* bwGrp = new QGroupBox("Network Bandwidth");
    bwGrp->setToolTip("Текущая пропускная способность сети (Mbps)");
    auto* bL = new QVBoxLayout(bwGrp);
    bandwidthChart_ = new QChart();
    bandwidthChart_->setBackgroundBrush(ThemePalette::crust());
    bandwidthChart_->legend()->hide();
    bandwidthChart_->setMargins(QMargins(0,0,0,0));

    bandwidthSeries_ = new QLineSeries();
    bandwidthSeries_->setPen(QPen(ThemePalette::sapphire(), 2));

    auto* bwArea = new QAreaSeries(bandwidthSeries_);
    QColor bwFill = ThemePalette::sapphire(); bwFill.setAlpha(60);
    bwArea->setBrush(bwFill); bwArea->setPen(QPen(ThemePalette::sapphire(), 2));
    bandwidthChart_->addSeries(bwArea);

    bwTimeAxis_ = new QDateTimeAxis(); bwTimeAxis_->setFormat("HH:mm:ss");
    bwTimeAxis_->setLabelsColor(ThemePalette::textSecondary());
    bwTimeAxis_->setGridLineColor(ThemePalette::surface0());
    bwAxis_ = new QValueAxis(); bwAxis_->setTitleText("Mbps");
    bwAxis_->setLabelsColor(ThemePalette::textSecondary());
    bwAxis_->setGridLineColor(ThemePalette::surface0());
    bandwidthChart_->addAxis(bwTimeAxis_, Qt::AlignBottom);
    bandwidthChart_->addAxis(bwAxis_, Qt::AlignLeft);
    bwArea->attachAxis(bwTimeAxis_); bwArea->attachAxis(bwAxis_);
    // bandwidthSeries_ needs axes too for data insertion
    bandwidthSeries_->attachAxis(bwTimeAxis_);
    bandwidthSeries_->attachAxis(bwAxis_);

    auto* bv = new QChartView(bandwidthChart_);
    bv->setRenderHint(QPainter::Antialiasing);
    bv->setStyleSheet("background: transparent; border: none;");
    bL->addWidget(bv);
    botLayout->addWidget(bwGrp, 1);

    auto* hmGrp = new QGroupBox("Port Activity Heatmap");
    hmGrp->setToolTip("Активность портов во времени. Цвет показывает интенсивность трафика");
    auto* hmL = new QVBoxLayout(hmGrp);
    heatmapWidget_ = new HeatmapWidget();
    hmL->addWidget(heatmapWidget_);
    botLayout->addWidget(hmGrp, 1);

    // Packet Size Distribution — п.5: инициализируем 5 нулями
    auto* sizeGrp = new QGroupBox("Packet Size Distribution");
    sizeGrp->setToolTip("Распределение пакетов по размерам (байт)");
    auto* sL = new QVBoxLayout(sizeGrp);
    packetSizeChart_ = new QChart();
    packetSizeChart_->setBackgroundBrush(ThemePalette::crust()); // п.7
    packetSizeChart_->legend()->hide();
    packetSizeChart_->setMargins(QMargins(0,0,0,0));
    packetSizeSeries_ = new QBarSeries();
    packetSizeSet_ = new QBarSet("Count");
    packetSizeSet_->setColor(ThemePalette::sapphire());
    // п.5: добавляем 5 нулевых значений, чтобы replace() работал
    *packetSizeSet_ << 0.0 << 0.0 << 0.0 << 0.0 << 0.0;
    packetSizeSeries_->append(packetSizeSet_);
    packetSizeChart_->addSeries(packetSizeSeries_);
    sizeCatAxis_ = new QBarCategoryAxis();
    sizeCatAxis_->append({"0-64", "65-128", "129-512", "513-1k", ">1k"});
    sizeCatAxis_->setLabelsColor(ThemePalette::textSecondary());
    sizeValAxis_ = new QValueAxis();
    sizeValAxis_->setLabelsColor(ThemePalette::textSecondary());
    sizeValAxis_->setGridLineColor(ThemePalette::surface0());
    packetSizeChart_->addAxis(sizeCatAxis_, Qt::AlignBottom);
    packetSizeChart_->addAxis(sizeValAxis_, Qt::AlignLeft);
    packetSizeSeries_->attachAxis(sizeCatAxis_);
    packetSizeSeries_->attachAxis(sizeValAxis_);
    auto* sv = new QChartView(packetSizeChart_);
    sv->setRenderHint(QPainter::Antialiasing);
    sv->setStyleSheet("background: transparent; border: none;");
    sL->addWidget(sv);
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
    
    timeAxis_->setLabelsColor(ThemePalette::textPrimary());
    ppsAxis_->setLabelsColor(ThemePalette::textPrimary());
    confAxis_->setLabelsColor(ThemePalette::textPrimary());
    
    if (bwTimeAxis_) bwTimeAxis_->setLabelsColor(ThemePalette::textPrimary());
    if (bwAxis_) bwAxis_->setLabelsColor(ThemePalette::textPrimary());
    if (sizeCatAxis_) sizeCatAxis_->setLabelsColor(ThemePalette::textPrimary());
    if (sizeValAxis_) sizeValAxis_->setLabelsColor(ThemePalette::textPrimary());
    
    if (bandwidthChart_) bandwidthChart_->setBackgroundBrush(ThemePalette::crust());
    if (packetSizeChart_) packetSizeChart_->setBackgroundBrush(ThemePalette::crust());
    
    if (bandwidthSeries_) bandwidthSeries_->setPen(QPen(ThemePalette::sapphire(), 2));
    if (packetSizeSet_) packetSizeSet_->setColor(ThemePalette::sapphire());

    if (cpuChart_) cpuChart_->legend()->setLabelBrush(QBrush(ThemePalette::textPrimary()));
    if (ramChart_) ramChart_->legend()->setLabelBrush(QBrush(ThemePalette::textPrimary()));
    if (trafficChart_) trafficChart_->legend()->setLabelBrush(QBrush(ThemePalette::textPrimary()));
    if (topPortsChart_) topPortsChart_->legend()->setLabelBrush(QBrush(ThemePalette::textPrimary()));
    
    // Also re-apply the global style to widgets because properties changed
    qApp->setStyleSheet(ThemePalette::globalStyleSheet());
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
    
    double bwVal = (result.totalBytes * 8.0) / 1000000.0 / dt;
    bandwidthSeries_->append(ms, bwVal);
    
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
        bwAxis_->setRange(0, std::max(0.05, maxVisibleBw * 1.15));
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