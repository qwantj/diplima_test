#include "CSVUtilsTests.hpp"
#include "common/CSVUtils.hpp"

void CSVUtilsTests::testSanitizeField() {
    // Normal text should not be changed
    QCOMPARE(CSVUtils::sanitizeField("Normal Text"), QString("Normal Text"));
    QCOMPARE(CSVUtils::sanitizeField("123.456"), QString("123.456"));
    QCOMPARE(CSVUtils::sanitizeField("2023-01-01"), QString("2023-01-01"));
    QCOMPARE(CSVUtils::sanitizeField(""), QString(""));

    // Dangerous characters at the start should be prefixed with a single quote
    QCOMPARE(CSVUtils::sanitizeField("=1+2"), QString("'=1+2"));
    QCOMPARE(CSVUtils::sanitizeField("+Positive"), QString("'+Positive"));
    QCOMPARE(CSVUtils::sanitizeField("-Negative"), QString("'-Negative"));
    QCOMPARE(CSVUtils::sanitizeField("@Username"), QString("'@Username"));
    QCOMPARE(CSVUtils::sanitizeField("\tTab"), QString("'\tTab"));
    QCOMPARE(CSVUtils::sanitizeField("\rReturn"), QString("'\rReturn"));

    // Dangerous characters NOT at the start should not be prefixed
    QCOMPARE(CSVUtils::sanitizeField("Result=Success"), QString("Result=Success"));
    QCOMPARE(CSVUtils::sanitizeField("1+1=2"), QString("1+1=2"));
}
