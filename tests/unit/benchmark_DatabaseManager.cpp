#include <QtTest>
#include <pqxx/pqxx>
#include <memory>
#include <string>

class BenchmarkDatabaseManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Connect to PostgreSQL
        try {
            const char* envPass = std::getenv("DDOS_DB_PASS");
            std::string dbPass = envPass ? envPass : "postgres";
            conn_ = std::make_unique<pqxx::connection>(
                "host=localhost port=5432 dbname=postgres user=postgres password=" + dbPass);
            
            if (!conn_->is_open()) {
                QSKIP("Cannot connect to PostgreSQL - skipping benchmark");
                return;
            }

            // Create test tables
            pqxx::work tx(*conn_);
            tx.exec("CREATE TABLE IF NOT EXISTS events_bench ("
                   "id SERIAL PRIMARY KEY, "
                   "session_id INTEGER, "
                   "timestamp TEXT, "
                   "label INTEGER, "
                   "confidence REAL, "
                   "pps REAL, "
                   "total_packets BIGINT, "
                   "features TEXT, "
                   "model_name TEXT"
                   ")");

            tx.exec("CREATE TABLE IF NOT EXISTS snapshots_bench ("
                   "id SERIAL PRIMARY KEY, "
                   "session_id INTEGER, "
                   "timestamp TEXT, "
                   "packets_per_s REAL, "
                   "total_packets BIGINT, "
                   "current_label INTEGER"
                   ")");
            tx.commit();
        } catch (const std::exception& e) {
            QSKIP(("Cannot connect to PostgreSQL: " + std::string(e.what())).c_str());
        }
    }

    void cleanupTestCase() {
        if (conn_ && conn_->is_open()) {
            try {
                pqxx::work tx(*conn_);
                tx.exec("DROP TABLE IF EXISTS events_bench");
                tx.exec("DROP TABLE IF EXISTS snapshots_bench");
                tx.commit();
                conn_->close();
            } catch (...) {}
        }
    }

    void cleanup() {
        if (!conn_ || !conn_->is_open()) return;
        try {
            pqxx::work tx(*conn_);
            tx.exec("DELETE FROM events_bench");
            tx.exec("DELETE FROM snapshots_bench");
            tx.commit();
        } catch (...) {}
    }

    void benchmarkRowByRowInsert() {
        if (!conn_ || !conn_->is_open()) QSKIP("No DB connection");
        const int NUM_RECORDS = 1000;

        QBENCHMARK {
            pqxx::work tx(*conn_);
            for (int i = 0; i < NUM_RECORDS; ++i) {
                tx.exec_params(
                    "INSERT INTO events_bench (session_id, timestamp, label, confidence, pps, total_packets, features, model_name) "
                    "VALUES ($1, $2, $3, $4, $5, $6, $7, $8)",
                    1, "2024-05-11 10:00:00", 0, 0.99f, 100.0, static_cast<int64_t>(i), "[1,2,3]", "model_v1"
                );
            }
            tx.commit();
        }
    }

    void benchmarkStreamToInsert() {
        if (!conn_ || !conn_->is_open()) QSKIP("No DB connection");
        const int NUM_RECORDS = 1000;

        QBENCHMARK {
            pqxx::work tx(*conn_);
            auto stream = pqxx::stream_to::table(tx, {"events_bench"},
                {"session_id", "timestamp", "label", "confidence", "pps", "total_packets", "features", "model_name"});

            for (int i = 0; i < NUM_RECORDS; ++i) {
                stream << std::make_tuple(
                    1, std::string("2024-05-11 10:00:00"), 0, 0.99f, 100.0, static_cast<int64_t>(i), 
                    std::string("[1,2,3]"), std::string("model_v1")
                );
            }
            stream.complete();
            tx.commit();
        }
    }

    void benchmarkThrottledStreamToInsert() {
        if (!conn_ || !conn_->is_open()) QSKIP("No DB connection");
        const int NUM_RECORDS = 5000;
        const int MAX_EVENTS_PER_FLUSH = 1000;

        QBENCHMARK {
            int recordsProcessed = 0;
            while (recordsProcessed < NUM_RECORDS) {
                int currentBatchSize = qMin(MAX_EVENTS_PER_FLUSH, NUM_RECORDS - recordsProcessed);

                pqxx::work tx(*conn_);
                auto stream = pqxx::stream_to::table(tx, {"events_bench"},
                    {"session_id", "timestamp", "label", "confidence", "pps", "total_packets", "features", "model_name"});

                for (int i = 0; i < currentBatchSize; ++i) {
                    stream << std::make_tuple(
                        1, std::string("2024-05-11 10:00:00"), 0, 0.99f, 100.0, 
                        static_cast<int64_t>(recordsProcessed + i), 
                        std::string("[1,2,3]"), std::string("model_v1")
                    );
                }
                stream.complete();
                tx.commit();

                recordsProcessed += currentBatchSize;
            }
        }
    }

    void benchmarkSnapshotRowByRowInsert() {
        if (!conn_ || !conn_->is_open()) QSKIP("No DB connection");
        const int NUM_RECORDS = 1000;

        QBENCHMARK {
            pqxx::work tx(*conn_);
            for (int i = 0; i < NUM_RECORDS; ++i) {
                tx.exec_params(
                    "INSERT INTO snapshots_bench (session_id, timestamp, packets_per_s, total_packets, current_label) "
                    "VALUES ($1, $2, $3, $4, $5)",
                    1, "2024-05-11 10:00:00", 100.5f, static_cast<int64_t>(i), 0
                );
            }
            tx.commit();
        }
    }

    void benchmarkSnapshotStreamToInsert() {
        if (!conn_ || !conn_->is_open()) QSKIP("No DB connection");
        const int NUM_RECORDS = 1000;

        QBENCHMARK {
            pqxx::work tx(*conn_);
            auto stream = pqxx::stream_to::table(tx, {"snapshots_bench"},
                {"session_id", "timestamp", "packets_per_s", "total_packets", "current_label"});

            for (int i = 0; i < NUM_RECORDS; ++i) {
                stream << std::make_tuple(
                    1, std::string("2024-05-11 10:00:00"), 100.5f, static_cast<int64_t>(i), 0
                );
            }
            stream.complete();
            tx.commit();
        }
    }

private:
    std::unique_ptr<pqxx::connection> conn_;
};

QTEST_MAIN(BenchmarkDatabaseManager)
#include "benchmark_DatabaseManager.moc"
