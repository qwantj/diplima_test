#include <QCoreApplication>
#include <QElapsedTimer>
#include <QThread>
#include <iostream>
#include <vector>
#include "common/DatabaseManager.hpp"
#include "common/AppLogger.hpp"

// We need an application instance to use QTimer and Qt SQL cleanly
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    // Initialize logger
    AppLogger::init();

    DatabaseManager db;
    const char* envPass = std::getenv("DDOS_DB_PASS");
    std::string dbPass = envPass ? envPass : "";

    bool ok = db.connectToDatabase("localhost", 5432, "ddos_detection_db", "postgres", QString::fromStdString(dbPass));
    if (!ok) {
        std::cerr << "Failed to connect to database. Make sure postgres is running." << std::endl;
        return 1;
    }

    std::cout << "Connected to database. Starting benchmark..." << std::endl;

    // Create a dummy session
    int sessionId = db.createSession("benchmark_iface", "benchmark_model");

    const int NUM_RECORDS = 50000;

    // 1. Benchmark Events
    std::cout << "Enqueuing " << NUM_RECORDS << " events..." << std::endl;
    for (int i = 0; i < NUM_RECORDS; ++i) {
        DetectionResult r;
        r.sessionId = sessionId;
        r.timestamp = QDateTime::currentDateTime();
        r.label = 0;
        r.confidence = 0.99f;
        r.pps = 1000.0;
        r.totalPackets = i + 1;
        r.features = {1.0, 2.0, 3.0};
        r.modelName = "benchmark_model";
        db.enqueueEvent(r);
    }

    QElapsedTimer timer;
    timer.start();

    // Call flush indirectly or wait for the async writer
    // To properly benchmark just the flush, we might need a friend class or to temporarily make flush public
    // Since we just want to measure, we can wait for the event queue to be empty
    while (db.getEventsForSession(sessionId).size() < NUM_RECORDS) {
        QThread::msleep(100);
    }
    qint64 elapsedMs = timer.elapsed();
    std::cout << "Time to process 50,000 events: " << elapsedMs << " ms" << std::endl;

    // 2. Benchmark Snapshots
    std::cout << "Enqueuing " << NUM_RECORDS << " snapshots..." << std::endl;
    for (int i = 0; i < NUM_RECORDS; ++i) {
        db.enqueueSnapshot(100.0, i+1, 0, sessionId);
    }

    timer.start();
    // Since there's no get method for snapshots, we just disconnect which flushes queues
    db.disconnect();
    qint64 snapshotElapsedMs = timer.elapsed();
    std::cout << "Time to process 50,000 snapshots (via disconnect flush): " << snapshotElapsedMs << " ms" << std::endl;

    return 0;
}
