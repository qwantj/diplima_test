#ifndef CSV_UTILS_TESTS_HPP
#define CSV_UTILS_TESTS_HPP

#include <QtTest/QtTest>

class CSVUtilsTests : public QObject {
    Q_OBJECT

private slots:
    void testSanitizeField();
};

#endif // CSV_UTILS_TESTS_HPP
