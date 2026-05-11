#ifndef CSV_UTILS_HPP
#define CSV_UTILS_HPP

#include <QString>

namespace CSVUtils {

/**
 * @brief Sanitize a string for CSV export to prevent Formula Injection (CSV Injection)
 * and handle standard CSV escaping (commas, quotes).
 *
 * 1. If the string contains commas, quotes or newlines, it is wrapped in double quotes.
 * 2. Internal double quotes are escaped as "".
 * 3. If the field starts with dangerous characters (=, +, -, @, \t, \r),
 *    it is prefixed with a single quote (') to ensure it's treated as text by spreadsheet software.
 *
 * @param text The input text to sanitize.
 * @return The sanitized and escaped text.
 */
inline QString sanitizeField(const QString& text) {
    if (text.isEmpty()) {
        return text;
    }

    QString processed = text;
    bool needsQuotes = false;

    // Check if it needs CSV quoting
    if (processed.contains(',') || processed.contains('"') || processed.contains('\n') || processed.contains('\r')) {
        needsQuotes = true;
        processed.replace("\"", "\"\"");
    }

    // Formula injection protection: check the FIRST character of the ACTUAL content
    // We apply this BEFORE wrapping in quotes if we want it to be part of the literal string
    static const QString dangerousChars = "=+-@\t\r";
    if (dangerousChars.contains(processed.at(0))) {
        processed.prepend("'");
    }

    if (needsQuotes) {
        processed = "\"" + processed + "\"";
    }

    return processed;
}

} // namespace CSVUtils

#endif // CSV_UTILS_HPP
