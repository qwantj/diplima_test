#ifndef APP_LOGGER_TESTS_HPP
#define APP_LOGGER_TESTS_HPP

#include <QtTest/QtTest>

class AppLoggerTests : public QObject {
    Q_OBJECT

private slots:
    void testSingleton();
    void testInitialization();
    void testLogging();
};

#endif // APP_LOGGER_TESTS_HPP
