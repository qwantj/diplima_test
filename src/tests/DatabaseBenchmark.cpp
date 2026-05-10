#include <QCoreApplication>
#include <QElapsedTimer>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <iostream>
#include <vector>

void runBenchmark(QSqlDatabase& db, bool optimized) {
    int numEvents = 50000;

    // Create temporary table for test
    QSqlQuery q(db);
    q.exec("DROP TABLE IF EXISTS events_bench");
    if (!q.exec("CREATE TABLE events_bench (id SERIAL PRIMARY KEY, session_id INTEGER, timestamp TIMESTAMP, label INTEGER, confidence REAL, pps REAL, total_packets BIGINT, features TEXT, model_name TEXT)")) {
        qDebug() << "Failed to create table:" << q.lastError();
        return;
    }

    QElapsedTimer timer;
    db.transaction();
    timer.start();

    if (!optimized) {
        // N+1 pattern
        for (int i = 0; i < numEvents; ++i) {
            QSqlQuery q_insert(db);
            q_insert.prepare("INSERT INTO events_bench (session_id, timestamp, label, confidence, pps, total_packets, features, model_name) VALUES (?,?,?,?,?,?,?,?)");
            q_insert.addBindValue(i);
            q_insert.addBindValue(QDateTime::currentDateTime());
            q_insert.addBindValue(1);
            q_insert.addBindValue(0.95);
            q_insert.addBindValue(100.5);
            q_insert.addBindValue(1000);
            q_insert.addBindValue("[0.1, 0.2, 0.3]");
            q_insert.addBindValue("model_v1");
            q_insert.exec();
        }
    } else {
        // Optimized pattern
        QSqlQuery q_insert(db);
        q_insert.prepare("INSERT INTO events_bench (session_id, timestamp, label, confidence, pps, total_packets, features, model_name) VALUES (?,?,?,?,?,?,?,?)");
        for (int i = 0; i < numEvents; ++i) {
            q_insert.bindValue(0, i);
            q_insert.bindValue(1, QDateTime::currentDateTime());
            q_insert.bindValue(2, 1);
            q_insert.bindValue(3, 0.95);
            q_insert.bindValue(4, 100.5);
            q_insert.bindValue(5, 1000);
            q_insert.bindValue(6, "[0.1, 0.2, 0.3]");
            q_insert.bindValue(7, "model_v1");
            q_insert.exec();
        }
    }

    db.commit();
    qint64 elapsed = timer.elapsed();

    std::cout << (optimized ? "Optimized" : "Unoptimized") << " benchmark: "
              << numEvents << " events inserted in " << elapsed << " ms." << std::endl;
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("localhost");
    db.setPort(5432);
    db.setDatabaseName("postgres"); // default
    db.setUserName("postgres"); // default
    db.setPassword("postgres"); // default

    if (!db.open()) {
        std::cerr << "Failed to open database. Benchmark aborted." << std::endl;
        std::cerr << db.lastError().text().toStdString() << std::endl;
        return 1;
    }

    std::cout << "Database opened successfully. Running benchmarks..." << std::endl;

    runBenchmark(db, false);
    runBenchmark(db, true);

    return 0;
}
