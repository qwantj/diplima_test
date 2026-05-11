#ifndef CSV_UTILS_HPP
#define CSV_UTILS_HPP

#include <QString>

namespace CSVUtils {

/**
 * @brief Sanitize a string for CSV export to prevent Formula Injection (CSV Injection).
 *
 * If the string starts with any dangerous characters (=, +, -, @, \t, \r),
 * it is prefixed with a single quote (') to ensure it's treated as text by spreadsheet software.
 *
 * @param text The input text to sanitize.
 * @return The sanitized text.
 */
inline QString sanitizeField(const QString& text) {
    if (text.isEmpty()) {
        return text;
    }

    // Characters that can trigger formula execution in spreadsheet applications
    static const QString dangerousChars = "=+-@\t\r";

    if (dangerousChars.contains(text.at(0))) {
        return "'" + text;
    }

    return text;
}

} // namespace CSVUtils

#endif // CSV_UTILS_HPP
