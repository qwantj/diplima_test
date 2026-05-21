#include <QCoreApplication>
#include <QElapsedTimer>
#include <QDebug>
#include <QDateTime>
#include <pqxx/pqxx>
#include <iostream>
#include <vector>
#include <sstream>

void runBenchmark(pqxx::connection& conn, bool optimized) {
    int numEvents = 50000;

    // Create temporary table for test
    {
        pqxx::work tx(conn);
        tx.exec("DROP TABLE IF EXISTS events_bench");
        tx.exec("CREATE TABLE events_bench (id SERIAL PRIMARY KEY, session_id INTEGER, timestamp TIMESTAMP, label INTEGER, confidence REAL, pps REAL, total_packets BIGINT, features TEXT, model_name TEXT)");
        tx.commit();
    }

    QElapsedTimer timer;
    timer.start();

    if (!optimized) {
        // N+1 pattern — individual INSERTs inside one transaction
        pqxx::work tx(conn);
        for (int i = 0; i < numEvents; ++i) {
            tx.exec_params(
                "INSERT INTO events_bench (session_id, timestamp, label, confidence, pps, total_packets, features, model_name) VALUES ($1,$2,$3,$4,$5,$6,$7,$8)",
                i,
                QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz").toStdString(),
                1,
                0.95,
                100.5,
                1000,
                "[0.1, 0.2, 0.3]",
                "model_v1"
            );
        }
        tx.commit();
    } else {
        // Optimized pattern — COPY via stream_to
        pqxx::work tx(conn);
        auto stream = pqxx::stream_to::table(tx, {"events_bench"},
            {"session_id", "timestamp", "label", "confidence", "pps", "total_packets", "features", "model_name"});

        for (int i = 0; i < numEvents; ++i) {
            stream << std::make_tuple(
                i,
                QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz").toStdString(),
                1,
                0.95,
                100.5,
                static_cast<int64_t>(1000),
                std::string("[0.1, 0.2, 0.3]"),
                std::string("model_v1")
            );
        }
        stream.complete();
        tx.commit();
    }

    qint64 elapsed = timer.elapsed();

    std::cout << (optimized ? "Optimized (COPY)" : "Unoptimized (N x INSERT)") << " benchmark: "
              << numEvents << " events inserted in " << elapsed << " ms." << std::endl;
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    try {
        pqxx::connection conn("host=localhost port=5432 dbname=postgres user=postgres password=postgres");

        if (!conn.is_open()) {
            std::cerr << "Failed to open database. Benchmark aborted." << std::endl;
            return 1;
        }

        std::cout << "Database opened successfully. Running benchmarks..." << std::endl;

        runBenchmark(conn, false);
        runBenchmark(conn, true);
    } catch (const std::exception& e) {
        std::cerr << "Failed to open database: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
