#include "test_SystemMetricsCollector.hpp"
#include "common/SystemMetricsCollector.hpp"

void TestSystemMetricsCollector::testValidData() {
    SystemMetricsCollector collector;
    SystemMetrics metrics = collector.collect();

#ifdef _WIN32
    // On Windows, the actual OS metrics are collected.
    QVERIFY(metrics.ramTotalBytes > 0);
    QVERIFY(metrics.ramUsedBytes <= metrics.ramTotalBytes);

    QVERIFY(metrics.ramUsagePercent >= 0.0);
    QVERIFY(metrics.ramUsagePercent <= 100.0);

    QVERIFY(metrics.cpuUsagePercent >= 0.0);
    QVERIFY(metrics.cpuUsagePercent <= 100.0);
#else
    // On non-Windows platforms, default values are returned.
    QCOMPARE(metrics.ramTotalBytes, static_cast<uint64_t>(0));
    QCOMPARE(metrics.ramUsedBytes, static_cast<uint64_t>(0));
    QCOMPARE(metrics.ramUsagePercent, 0.0);
    QCOMPARE(metrics.cpuUsagePercent, 0.0);
#endif
}

void TestSystemMetricsCollector::testConsistency() {
    SystemMetricsCollector collector;

    // First call to initialize
    SystemMetrics m1 = collector.collect();

    // Slight delay to allow time to pass for CPU metrics measurement
    QTest::qWait(100);

    // Second call should also be valid and shouldn't crash
    SystemMetrics m2 = collector.collect();

#ifdef _WIN32
    QVERIFY(m2.ramTotalBytes > 0);
    QVERIFY(m2.ramUsedBytes <= m2.ramTotalBytes);
    QVERIFY(m2.cpuUsagePercent >= 0.0);
    QVERIFY(m2.cpuUsagePercent <= 100.0);
#endif
}
