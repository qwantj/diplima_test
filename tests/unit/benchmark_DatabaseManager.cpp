#include <QtTest>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariantList>

class BenchmarkDatabaseManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Setup SQLite memory database
        db_ = QSqlDatabase::addDatabase("QSQLITE", "benchmark_db");
        db_.setDatabaseName(":memory:");
        QVERIFY(db_.open());

        QSqlQuery q(db_);
        q.exec("CREATE TABLE events ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "session_id INTEGER, "
               "timestamp TEXT, "
               "label INTEGER, "
               "confidence REAL, "
               "pps REAL, "
               "total_packets INTEGER, "
               "features TEXT, "
               "model_name TEXT"
               ")");
    }

    void cleanupTestCase() {
        db_.close();
        QSqlDatabase::removeDatabase("benchmark_db");
    }

    void cleanup() {
        QSqlQuery q(db_);
        q.exec("DELETE FROM events");
    }

    void benchmarkRowByRowInsert() {
        const int NUM_RECORDS = 1000;

        QBENCHMARK {
            db_.transaction();
            QSqlQuery q(db_);
            q.prepare("INSERT INTO events (session_id, timestamp, label, confidence, pps, total_packets, features, model_name) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");

            for (int i = 0; i < NUM_RECORDS; ++i) {
                q.bindValue(0, 1);
                q.bindValue(1, "2024-05-11 10:00:00");
                q.bindValue(2, 0);
                q.bindValue(3, 0.99f);
                q.bindValue(4, 100.0);
                q.bindValue(5, i);
                q.bindValue(6, "[1,2,3]");
                q.bindValue(7, "model_v1");
                q.exec();
            }
            db_.commit();
        }
    }

    void benchmarkBatchInsert() {
        const int NUM_RECORDS = 1000;

        QBENCHMARK {
            QVariantList sessionIds, timestamps, labels, confidences, ppsValues, totalPackets, features, modelNames;

            // Simulation of our new reserve optimization
            sessionIds.reserve(NUM_RECORDS);
            timestamps.reserve(NUM_RECORDS);
            labels.reserve(NUM_RECORDS);
            confidences.reserve(NUM_RECORDS);
            ppsValues.reserve(NUM_RECORDS);
            totalPackets.reserve(NUM_RECORDS);
            features.reserve(NUM_RECORDS);
            modelNames.reserve(NUM_RECORDS);

            for (int i = 0; i < NUM_RECORDS; ++i) {
                sessionIds << 1;
                timestamps << "2024-05-11 10:00:00";
                labels << 0;
                confidences << 0.99f;
                ppsValues << 100.0;
                totalPackets << i;
                features << "[1,2,3]";
                modelNames << "model_v1";
            }

            db_.transaction();
            QSqlQuery q(db_);
            q.prepare("INSERT INTO events (session_id, timestamp, label, confidence, pps, total_packets, features, model_name) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");

            q.bindValue(0, sessionIds);
            q.bindValue(1, timestamps);
            q.bindValue(2, labels);
            q.bindValue(3, confidences);
            q.bindValue(4, ppsValues);
            q.bindValue(5, totalPackets);
            q.bindValue(6, features);
            q.bindValue(7, modelNames);

            q.execBatch();
            db_.commit();
        }
    }

    void benchmarkThrottledBatchInsert() {
        const int NUM_RECORDS = 5000;
        const int MAX_EVENTS_PER_FLUSH = 1000;

        QBENCHMARK {
            int recordsProcessed = 0;
            while (recordsProcessed < NUM_RECORDS) {
                int currentBatchSize = qMin(MAX_EVENTS_PER_FLUSH, NUM_RECORDS - recordsProcessed);

                QVariantList sessionIds, timestamps, labels, confidences, ppsValues, totalPackets, features, modelNames;
                sessionIds.reserve(currentBatchSize);
                timestamps.reserve(currentBatchSize);
                labels.reserve(currentBatchSize);
                confidences.reserve(currentBatchSize);
                ppsValues.reserve(currentBatchSize);
                totalPackets.reserve(currentBatchSize);
                features.reserve(currentBatchSize);
                modelNames.reserve(currentBatchSize);

                for (int i = 0; i < currentBatchSize; ++i) {
                    sessionIds << 1;
                    timestamps << "2024-05-11 10:00:00";
                    labels << 0;
                    confidences << 0.99f;
                    ppsValues << 100.0;
                    totalPackets << (recordsProcessed + i);
                    features << "[1,2,3]";
                    modelNames << "model_v1";
                }

                db_.transaction();
                QSqlQuery q(db_);
                q.prepare("INSERT INTO events (session_id, timestamp, label, confidence, pps, total_packets, features, model_name) "
                          "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");

                q.bindValue(0, sessionIds);
                q.bindValue(1, timestamps);
                q.bindValue(2, labels);
                q.bindValue(3, confidences);
                q.bindValue(4, ppsValues);
                q.bindValue(5, totalPackets);
                q.bindValue(6, features);
                q.bindValue(7, modelNames);

                q.execBatch();
                db_.commit();

                recordsProcessed += currentBatchSize;
            }
        }
    }

private:
    QSqlDatabase db_;
};

QTEST_MAIN(BenchmarkDatabaseManager)
#include "benchmark_DatabaseManager.moc"
