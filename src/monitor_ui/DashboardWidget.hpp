/**
 * @file DashboardWidget.hpp
 * @brief Главная панель мониторинга (заголовок).
 *
 * Назначение: Отображение графиков трафика, системных метрик и управление параметрами защиты.
 * Входные данные: Объекты DetectionResult от коллектора.
 * Результаты: Визуализация состояния сетевой безопасности в реальном времени.
 * Метод решения: Использование Qt Charts для отрисовки динамических данных и кастомных виджетов (Heatmap, Topology).
 * Программист: Дерюга А.А.
 * Дата написания: 26.05.2026
 * Версия: 1.0
 */

#pragma once // Директива защиты от повторного включения

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainterPath>
#include <QCheckBox>
#include <QPushButton>
#include <QTimer>
#include <QTableWidget>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QBarCategoryAxis>
#include <QPainter>
#include <vector>
#include <deque>
#include <map>

#include "common/Protocol.hpp"
#include "common/SystemMetricsCollector.hpp"
#include "monitor_ui/ThemePalette.hpp"

/**
 * @class InteractiveChartView
 * @brief Представление графика с поддержкой масштабирования и перемещения.
 */
class InteractiveChartView : public QChartView {
  Q_OBJECT
public:
  explicit InteractiveChartView(QChart* chart, QWidget* parent = nullptr);
signals:
  void userInteracted();
  void mouseMoved(const QPoint& pos);
  void mouseLeft();
protected:
  void wheelEvent(QWheelEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void leaveEvent(QEvent* event) override;
private:
  bool dragging_ = false;
  QPointF lastMousePos_;
};

/**
 * @class SmartTooltip
 * @brief Всплывающая подсказка с поддержкой цветовых индикаторов.
 */
class SmartTooltip : public QFrame {
  Q_OBJECT
public:
  explicit SmartTooltip(QWidget* parent = nullptr);
  void updateContent(const QString& title, const QList<std::pair<QColor, QString>>& items, const QString& footer = "");
private:
  QLabel* label_ = nullptr;
};

/**
 * @class AlertGridWidget
 * @brief Виджет сетки состояния SLO (Service Level Objectives).
 */
class AlertGridWidget : public QWidget {
  Q_OBJECT
public:
  explicit AlertGridWidget(QWidget* parent = nullptr);
  void addHealthPoint(bool ok);
protected:
  void paintEvent(QPaintEvent*) override;
private:
  std::deque<bool> healthHistory_;
};

/**
 * @class NetworkTopologyWidget
 * @brief Визуализация топологии входящих потоков.
 */
class NetworkTopologyWidget : public QWidget {
  Q_OBJECT
public:
  explicit NetworkTopologyWidget(QWidget* parent = nullptr);
  void updateTopology(const std::vector<std::pair<std::string, uint64_t>>& targets, double ingressMbps);
protected:
  void paintEvent(QPaintEvent*) override;
private:
  std::vector<std::pair<std::string, uint64_t>> targets_;
  double ingressMbps_ = 0.0;
};

/**
 * @class HeatmapWidget
 * @brief Тепловая карта активности сетевых портов.
 */
class HeatmapWidget : public QWidget {
  Q_OBJECT
public:
  explicit HeatmapWidget(QWidget* parent = nullptr);
  void addData(const QDateTime& time, const std::map<uint16_t, uint64_t>& portData);
protected:
  void paintEvent(QPaintEvent*) override;
private:
  struct HeatPoint { QDateTime t; uint16_t port; double intensity; };
  std::deque<HeatPoint> points_;
  QDateTime minTime_;
  QDateTime maxTime_;
};

/**
 * @class DonutChartView
 * @brief Кольцевая диаграмма с текстом в центре.
 */
class DonutChartView : public QChartView {
  Q_OBJECT
public:
  explicit DonutChartView(QChart* chart, QWidget* parent = nullptr);
  void setCenterText(const QString& text);
protected:
  void paintEvent(QPaintEvent* event) override;
private:
  QString centerText_;
};

/**
 * @class ColorCheckBox
 * @brief Кастомный чекбокс с цветовым маркером (легенда графика).
 */
class ColorCheckBox : public QWidget {
  Q_OBJECT
public:
  explicit ColorCheckBox(const QString& label, const QColor& color, QWidget* parent = nullptr);
  bool isChecked() const { return checked_; }
  void setChecked(bool v) { checked_ = v; update(); }
signals:
  void toggled(bool checked);
protected:
  void paintEvent(QPaintEvent*) override;
  void mousePressEvent(QMouseEvent*) override;
private:
  QString label_;
  QColor  color_;
  bool    checked_ = true;
};

/**
 * @class DashboardWidget
 * @brief Основной виджет информационной панели.
 */
class DashboardWidget : public QWidget {
  Q_OBJECT
public:
  explicit DashboardWidget(QWidget* parent = nullptr);

signals:
  void bpfToggled(bool enabled);
  void pcapToggled(bool enabled);

public slots:
  // Обновление данных в реальном времени
  void updateRealtime(const DetectionResult& result, uint64_t totalPackets);
  void updateSnapshot(float pps, uint64_t totalPackets, int currentLabel);
  void updateConnectionStatus(bool connected);
  void loadHistory(const std::vector<DetectionResult>& events);
  
  // Управление оформлением
  void applyTheme(ThemeMode mode);
  void resetZoom();
  
  // Доступ к вкладкам аналитики
  QWidget* getTabAnalytics() const;
  QWidget* getTabSystem() const;

private slots:
  void onUserInteracted();
  void updateSystemMetrics();
  void onChartHover(const QPoint& pos);
  void onChartLeave();

private:
  void setupUI();
  void setupDashboard();
  void setupAnalytics();
  void addDataPoint(const DetectionResult& result);
  void updateDonuts(double cpu, double ram);

  SmartTooltip* chartTooltip_ = nullptr;
  
  // Элементы статусной строки
  QLabel* lblCollector_ = nullptr;
  QLabel* lblTotalPackets_ = nullptr;
  QLabel* lblPps_ = nullptr;
  QLabel* lblDropRate_ = nullptr;
  QLabel* lblSources_ = nullptr;
  QLabel* lblFlows_ = nullptr;
  QCheckBox* bpfCheckbox_ = nullptr;
  QLabel* lblModel_ = nullptr;
  QLabel* lblProbability_ = nullptr;
  QLabel* lblStatus_ = nullptr;

  // Основной график PPS
  QChart* ppsChart_ = nullptr;
  QLineSeries* ppsSeries_ = nullptr;
  QAreaSeries* ppsArea_ = nullptr;
  
  QLineSeries* zeroSeries_ = nullptr;
  QLineSeries* tcpUpper_ = nullptr;
  QAreaSeries* tcpArea_ = nullptr;
  QLineSeries* udpUpper_ = nullptr;
  QAreaSeries* udpArea_ = nullptr;
  QLineSeries* icmpUpper_ = nullptr;
  QAreaSeries* icmpArea_ = nullptr;
  
  QLineSeries* otherSeries_ = nullptr;
  QAreaSeries* otherArea_ = nullptr;
  QLineSeries* attackConfidenceUpper_ = nullptr;
  
  QDateTimeAxis* timeAxis_ = nullptr;
  QValueAxis* ppsAxis_ = nullptr;
  QValueAxis* confAxis_ = nullptr;
  InteractiveChartView* ppsChartView_ = nullptr;
  QPushButton* resetBtn_ = nullptr;

  // Чекбоксы слоев графика
  ColorCheckBox* cbPps_  = nullptr;
  ColorCheckBox* cbTcp_  = nullptr;
  ColorCheckBox* cbUdp_  = nullptr;
  ColorCheckBox* cbIcmp_ = nullptr;
  ColorCheckBox* cbOther_= nullptr;
  ColorCheckBox* cbConf_ = nullptr;

  // Кольцевые диаграммы ресурсов
  QChart* cpuChart_ = nullptr;
  QPieSeries* cpuPie_ = nullptr;
  QLabel* cpuTitle_ = nullptr;
  DonutChartView* cpuView_ = nullptr;
  
  QChart* ramChart_ = nullptr;
  QPieSeries* ramPie_ = nullptr;
  QLabel* ramTitle_ = nullptr;
  DonutChartView* ramView_ = nullptr;
  
  QChart* trafficChart_ = nullptr;
  QPieSeries* trafficPie_ = nullptr;
  QLabel* trafficTitle_ = nullptr;
  DonutChartView* trafficView_ = nullptr;

  // Виджеты вкладки аналитики
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

  // Сборщик системных метрик
  SystemMetricsCollector metricsCollector_;
  QTimer* metricsTimer_ = nullptr;

  // Вкладки интерфейса
  QWidget* tabSystem_ = nullptr;
  QWidget* tabAnalytics_ = nullptr;

  // История данных для графиков
  std::deque<QDateTime> timeHistory_;
  static constexpr int MAX_CHART_POINTS = 300;
  bool userInteracting_ = false;
  QTimer* autoScrollTimer_ = nullptr;
  uint64_t totalAttack_ = 0;
  uint64_t totalNormal_ = 0;
  bool collectorConnected_ = false;
};
