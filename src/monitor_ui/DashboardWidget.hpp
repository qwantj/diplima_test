#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QTimer>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QStackedBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QPainter>
#include <vector>
#include <deque>

#include "common/Protocol.hpp"
#include "common/SystemMetricsCollector.hpp"
#include "monitor_ui/ThemePalette.hpp"

// --- Helper widgets defined in header ---

class HeatmapWidget : public QWidget {
    Q_OBJECT
public:
    explicit HeatmapWidget(QWidget* parent = nullptr);
    void setData(const std::map<uint16_t, uint64_t>& portData);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    std::vector<std::pair<uint16_t, double>> data_;
};

class AlertGridWidget : public QWidget {
    Q_OBJECT
public:
    explicit AlertGridWidget(QWidget* parent = nullptr);
    void updateMetrics(double pps, int label, float confidence, double cpuPercent, double ramPercent);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    struct Cell {
        QString name;
        double value = 0;
        double threshold = 0;
        bool alert = false;
    };
    std::vector<Cell> cells_;
};

class InteractiveChartView : public QChartView {
    Q_OBJECT
public:
    explicit InteractiveChartView(QChart* chart, QWidget* parent = nullptr);
signals:
    void userInteracted();
protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
private:
    bool dragging_ = false;
    QPointF lastMousePos_;
};

// --- Main Dashboard ---

class DashboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit DashboardWidget(QWidget* parent = nullptr);

signals:
    void bpfToggled(bool enabled);

public slots:
    void updateRealtime(const DetectionResult& result, uint64_t totalPackets);
    void updateSnapshot(float pps, uint64_t totalPackets, int currentLabel);
    void updateConnectionStatus(bool connected);
    void loadHistory(const std::vector<DetectionResult>& events);
    void applyTheme(ThemeMode mode);
    void resetZoom();
    QWidget* getTabAnalytics() const;
    QWidget* getTabSystem() const;

private slots:
    void onUserInteracted();

private:
    void setupUI();
    void setupCharts();
    void addDataPoint(const DetectionResult& result);
    void updateSummaryCards(const DetectionResult& result, uint64_t totalPackets);
    void updateProtocolPie(const DetectionResult& result);
    void updateSystemMetrics();

    // Summary cards
    QLabel* lblPps_ = nullptr;
    QLabel* lblLabel_ = nullptr;
    QLabel* lblConfidence_ = nullptr;
    QLabel* lblTotalPackets_ = nullptr;
    QLabel* lblConnectionStatus_ = nullptr;

    // Charts
    QChart* ppsChart_ = nullptr;
    QLineSeries* ppsSeries_ = nullptr;
    QLineSeries* tcpSeries_ = nullptr;
    QLineSeries* udpSeries_ = nullptr;
    QLineSeries* icmpSeries_ = nullptr;
    QAreaSeries* attackArea_ = nullptr;
    QLineSeries* attackUpper_ = nullptr;
    QDateTimeAxis* timeAxis_ = nullptr;
    QValueAxis* ppsAxis_ = nullptr;
    InteractiveChartView* ppsChartView_ = nullptr;

    // Protocol pie
    QChart* protocolChart_ = nullptr;
    QPieSeries* protocolPie_ = nullptr;
    QChartView* protocolChartView_ = nullptr;

    // Heatmap & alert grid
    HeatmapWidget* heatmapWidget_ = nullptr;
    AlertGridWidget* alertGridWidget_ = nullptr;

    // System metrics
    QLabel* lblCpu_ = nullptr;
    QLabel* lblRam_ = nullptr;
    SystemMetricsCollector metricsCollector_;
    QTimer* metricsTimer_ = nullptr;

    // BPF control
    QCheckBox* bpfCheckbox_ = nullptr;

    // Tab widgets for analytics and system
    QWidget* tabAnalytics_ = nullptr;
    QWidget* tabSystem_ = nullptr;

    // Data history
    std::deque<QDateTime> timeHistory_;
    static constexpr int MAX_CHART_POINTS = 300;

    // Auto-scroll
    bool userInteracting_ = false;
    QTimer* autoScrollTimer_ = nullptr;
};
