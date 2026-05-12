#include "AppLoggerTests.hpp"
#include "common/AppLogger.hpp"
#include <QFile>

void AppLoggerTests::testSingleton() {
    auto& logger1 = AppLogger::get();
    auto& logger2 = AppLogger::get();
    QCOMPARE(logger1.get(), logger2.get());
    QVERIFY(logger1 != nullptr);
}

void AppLoggerTests::testInitialization() {
    AppLogger::reset();

    QString logFileName = "test_init.log";
    if (QFile::exists(logFileName)) {
        QFile::remove(logFileName);
    }

    AppLogger::init(logFileName.toStdString());

    auto& logger = AppLogger::get();
    QVERIFY(logger != nullptr);
    QCOMPARE(logger->name(), std::string("ddos"));

    // Write something to ensure file is created
    logger->info("Test initialization");
    logger->flush();

    // spdlog might not create the file immediately if it's buffered,
    // but basic_file_sink usually does.
    QVERIFY(QFile::exists(logFileName));

    AppLogger::reset();
    QFile::remove(logFileName);
}

void AppLoggerTests::testLogging() {
    AppLogger::init("test_logging.log");
    auto& logger = AppLogger::get();

    // Ensure these don't crash
    logger->info("Test info message");
    logger->warn("Test warning message");
    logger->error("Test error message");
    logger->debug("Test debug message");

    QVERIFY(true); // If we reached here, it didn't crash

    AppLogger::reset();
    QFile::remove("test_logging.log");
}
