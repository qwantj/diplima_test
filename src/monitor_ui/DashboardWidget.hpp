#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QTimer>
#include <QTableWidget>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QScatterSeries>
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

class HeatmapWidget : public QWidget {
    Q_OBJECT
public:
    explicit HeatmapWidget(QWidget* parent = nullptr);
    void addData(const QDateTime& time, const std::map<uint16_t, uint64_t>& portData);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    struct Point { QDateTime t; uint16_t port; double intensity; };
    std::deque<Point> points_;
    QDateTime minTime_, maxTime_;
};

class NetworkTopologyWidget : public QWidget {
    Q_OBJECT
public:
    explicit NetworkTopologyWidget(QWidget* parent = nullptr);
    void updateTopology(const std::vector<std::pair<std::string, uint64_t>>& targets, double ingressMbps);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    std::vector<std::pair<std::string, uint64_t>> targets_;
    double ingressMbps_ = 0;
};

class AlertGridWidget : public QWidget {
    Q_OBJECT
public:
    explicit AlertGridWidget(QWidget* parent = nullptr);
    void addHealthPoint(bool ok);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    std::deque<bool> healthHistory_; // 32 items for 4x8
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
    void updateSystemMetrics();

private:
    void setupUI();
    void setupDashboard();
    void setupAnalytics();
    
    void addDataPoint(const DetectionResult& result);
    void updateDonuts(double cpu, double ram);

    // -- Dashboard Overview --
    QWidget* tabSystem_ = nullptr;
    QLabel* lblCollector_ = nullptr;
    QLabel* lblTotalPackets_ = nullptr;
    QLabel* lblPps_ = nullptr;
    QLabel* lblDropRate_ = nullptr;
    QLabel* lblSources_ = nullptr;
    QCheckBox* bpfCheckbox_ = nullptr;
    QLabel* lblModel_ = nullptr;
    QLabel* lblProbability_ = nullptr;
    QLabel* lblStatus_ = nullptr;

    // -- Dashboard Main Chart --
    QChart* ppsChart_ = nullptr;
    QLineSeries* ppsSeries_ = nullptr;
    QLineSeries* tcpSeries_ = nullptr;
    QLineSeries* udpSeries_ = nullptr;
    QLineSeries* icmpSeries_ = nullptr;
    QLineSeries* otherSeries_ = nullptr;
    QAreaSeries* attackConfidenceArea_ = nullptr;
    QLineSeries* attackConfidenceUpper_ = nullptr;
    QDateTimeAxis* timeAxis_ = nullptr;
    QValueAxis* ppsAxis_ = nullptr;
    QValueAxis* confAxis_ = nullptr; // Right axis 0-100
    InteractiveChartView* ppsChartView_ = nullptr;

    // -- Dashboard Donuts --
    QChart* cpuChart_ = nullptr;
    QPieSeries* cpuPie_ = nullptr;
    QLabel* cpuTitle_ = nullptr;
    QChart* ramChart_ = nullptr;
    QPieSeries* ramPie_ = nullptr;
    QLabel* ramTitle_ = nullptr;
    QChart* trafficChart_ = nullptr;
    QPieSeries* trafficPie_ = nullptr;
    QLabel* trafficTitle_ = nullptr;

    // -- Deep Analytics Tab --
    QWidget* tabAnalytics_ = nullptr;
    AlertGridWidget* sloHealth_ = nullptr;
    NetworkTopologyWidget* topologyWidget_ = nullptr;
    
    QTableWidget* tableSources_ = nullptr;
    QTableWidget* tableTargets_ = nullptr;
    
    QChart* topPortsChart_ = nullptr;
    QPieSeries* topPortsPie_ = nullptr;
    
    QChart* bandwidthChart_ = nullptr;
    QLineSeries* bandwidthSeries_ = nullptr;
    QDateTimeAxis* bwTimeAxis_ = nullptr;
    QValueAxis* bwAxis_ = nullptr;

    HeatmapWidget* heatmapWidget_ = nullptr;
    
    QChart* packetSizeChart_ = nullptr;
    QBarSeries* packetSizeSeries_ = nullptr;
    QBarSet* packetSizeSet_ = nullptr;
    QBarCategoryAxis* sizeCatAxis_ = nullptr;
    QValueAxis* sizeValAxis_ = nullptr;

    // Data 
    SystemMetricsCollector metricsCollector_;
    QTimer* metricsTimer_ = nullptr;
    std::deque<QDateTime> timeHistory_;
    uint64_t totalNormal_ = 0;
    uint64_t totalAttack_ = 0;
    
    bool userInteracting_ = false;
    QTimer* autoScrollTimer_ = nullptr;
};