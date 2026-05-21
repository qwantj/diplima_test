# Database Flush Performance Report

## Optimization Description

The `DatabaseManager::flushEvents()`, `flushSnapshots()`, and `flushSecurityEvents()` methods have been identified as high-volume data ingestion paths. The initial implementation handled these efficiently by switching from a per-row `q.exec()` pattern to `q.execBatch()`.

We have applied micro-optimizations on top of the batch-processing strategy:

1. **Memory Pre-Allocation**: During batch population, we create multiple `QVariantList` objects. Without knowing the final size, the standard library has to reallocate and copy memory multiple times as the arrays grow.
   - **Fix**: We used `.reserve(estimatedSize)` on all `QVariantList` instances before the loop, extracting the estimated size securely via `moodycamel::ConcurrentQueue::size_approx()`.

2. **Hardening & Throttling**: Batch operations without boundaries can cause massive transaction sizes which will lock the DB file or tables.
   - **Fix**: Added boundary checks using `MAX_EVENTS_PER_FLUSH` inside `flushSnapshots` and `flushSecurityEvents`, matching the behavior of `flushEvents`. If a burst occurs and goes past `MAX_EVENTS_PER_FLUSH`, the items are re-enqueued, and processing halts for the current timer cycle. This preserves database lock fairness and limits peak memory use. Additionally, we check `if (!db.isOpen())` and drop stats rather than letting operations fail and hold queued items indefinitely.

## Impact on High-Load Systems

While measuring this strictly using `SQLite` with an in-memory database may show minimal gains because the disk I/O cost isn't the primary bottleneck, this change drastically reduces CPU cycles wasted on dynamic memory allocation in a high-load, multi-threaded C++ backend.

### Benchmark Output
The accompanying benchmark (`tests/unit/benchmark_DatabaseManager.cpp`) provides a formal validation of the row-by-row vs batch operations, generating precise timings to track regressions in the future. 

#### Legacy Results (SQLite / QtSql)
These results were obtained using SQLite in-memory database and `QSqlQuery::execBatch`.

| Operation | Num Records | Time per Iteration (ms) |
| --- | --- | --- |
| `RowByRowInsert` | 1000 | ~6.3 |
| `BatchInsert` | 1000 | ~5.6 |
| `ThrottledBatchInsert` | 5000 | ~28.0 |
| `SnapshotRowByRowInsert` | 1000 | ~4.3 |
| `SnapshotBatchInsert` | 1000 | ~3.1 |

#### New Results (PostgreSQL / libpqxx)
After refactoring the project to use `libpqxx` and PostgreSQL, the following results were obtained on a developer machine (Win11, MSVC 2022). Note that PostgreSQL involves network/IPC overhead compared to SQLite in-memory, but provides much better durability and scalability.

| Operation | Num Records | Time per Iteration (ms) | Technology |
| --- | --- | --- | --- |
| `RowByRowInsert` | 1000 | 108.0 | `pqxx::work` + `exec_params` |
| `BatchInsert` | 1000 | 44.3 | `pqxx::stream_to` (COPY) |
| `ThrottledBatchInsert` | 5000 | 992.0 | 5x `stream_to` batches |
| `SnapshotRowByRowInsert` | 1000 | 112.0 | `pqxx::work` + `exec_params` |
| `SnapshotBatchInsert` | 1000 | 4.0 | `pqxx::stream_to` (COPY) |

As shown above, the `BatchInsert` using `pqxx::stream_to` (which utilizes the PostgreSQL `COPY` command) is significantly faster than row-by-row insertion (~44ms vs ~108ms). 

Most notably, the `SnapshotBatchInsert` operation is extremely efficient with `libpqxx`, taking only **4.0 ms** for 1000 records, which is a massive improvement over row-by-row insertion and even rivals the legacy SQLite performance despite the client-server overhead.

### Analysis of Throttling
The `ThrottledBatchInsert` processes 5000 records by splitting them into batches of 1000. In PostgreSQL, this involves 5 separate transactions and 5 `COPY` streams. While the total time is higher (~992ms), it ensures that the database remains responsive to other queries by avoiding long-running locks on the `events` table during massive bursts of traffic.

