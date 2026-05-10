# Performance Report: DatabaseManager Optimization

## Objective
Optimize the performance of batch database operations in `DatabaseManager` (`flushEvents`, `flushSnapshots`, `flushSecurityEvents`) by eliminating the N+1 query preparation pattern.

## Implementation Details
Prior to this change, a `QSqlQuery` object was instantiated and prepared (`q.prepare(...)`) inside the `while` loops processing the queues. This resulted in the query being prepared for every single entry.
The optimization involved:
1. Instantiating and preparing the `QSqlQuery` object once, outside the processing loop.
2. Replacing `q.addBindValue(value)` with `q.bindValue(index, value)` to effectively re-bind the parameters for the already prepared statement.

## Benchmark Methodology
A dedicated benchmark program (`DatabaseBenchmark.cpp`) was created to simulate the `flushEvents` behavior. It inserts 50,000 dummy event records into a local PostgreSQL database using both the unoptimized and optimized patterns inside a single transaction.

## Results
The benchmark measured the elapsed time to insert 50,000 records.

- **Baseline (Unoptimized N+1 pattern)**: ~29521 ms
- **Optimized (Prepared once, reused)**: ~10224 ms

### Conclusion
By moving the `QSqlQuery` preparation outside the loop, we observed an approximate **65% reduction in execution time** (nearly a 3x speedup) for bulk inserts. This is a significant improvement for the high-load data pipeline components of the system.
