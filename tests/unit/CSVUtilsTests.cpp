#include "CSVUtilsTests.hpp"
#include "common/CSVUtils.hpp"

void CSVUtilsTests::testSanitizeField() {
    // Normal text
    QCOMPARE(CSVUtils::sanitizeField("Normal Text"), QString("Normal Text"));
    QCOMPARE(CSVUtils::sanitizeField("123.456"), QString("123.456"));

    // Formula injection
    QCOMPARE(CSVUtils::sanitizeField("=1+2"), QString("'=1+2"));
    QCOMPARE(CSVUtils::sanitizeField("+Positive"), QString("'+Positive"));

    // CSV Escaping (Quotes)
    QCOMPARE(CSVUtils::sanitizeField("Text with \"quotes\""), QString("\"Text with \"\"quotes\"\"\""));

    // CSV Escaping (Comma)
    QCOMPARE(CSVUtils::sanitizeField("City, Country"), QString("\"City, Country\""));

    // Both Formula and CSV Escaping
    // If it starts with '=', it gets "'". If it contains ',', it gets wrapped in quotes.
    // Result: "'=City, Country" -> wrapped -> "\"'=City, Country\""
    QCOMPARE(CSVUtils::sanitizeField("=City, Country"), QString("\"'=City, Country\""));
}
