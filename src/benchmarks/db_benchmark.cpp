#include <iostream>
#include <chrono>
#include <vector>
#include <string>

// Mocking DatabaseManager flush behavior to demonstrate optimization
struct MockEntry {
    int id;
    std::string data;
};

void slowFlush(const std::vector<MockEntry>& entries) {
    for (const auto& entry : entries) {
        // Simulating QSqlQuery q; q.prepare(...)
        std::string query = "INSERT INTO events VALUES (?, ?)";
        // Simulate binding and execution
        (void)entry;
    }
}

void fastFlush(const std::vector<MockEntry>& entries) {
    // Simulating QSqlQuery q; q.prepare(...) outside
    std::string query = "INSERT INTO events VALUES (?, ?)";
    for (const auto& entry : entries) {
        // Simulate binding and execution
        (void)entry;
    }
}

int main() {
    const int count = 1000000;
    std::vector<MockEntry> entries(count, {1, "test_data"});

    auto start = std::chrono::high_resolution_clock::now();
    slowFlush(entries);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "Slow flush time: " << diff.count() << " s\n";

    start = std::chrono::high_resolution_clock::now();
    fastFlush(entries);
    end = std::chrono::high_resolution_clock::now();
    diff = end - start;
    std::cout << "Fast flush time: " << diff.count() << " s\n";

    return 0;
}
