#include "monitor_ui/ThemePalette.hpp"
#include <QStyle>

ThemeMode ThemePalette::currentMode_ = ThemeMode::Dark;

// ═══════════════════════════════════════════════════════════════
//  apply() — sets QPalette using Catppuccin Mocha (dark) / Latte (light)
// ═══════════════════════════════════════════════════════════════
void ThemePalette::apply(ThemeMode mode) {
    currentMode_ = mode;

    QPalette pal;
    if (mode == ThemeMode::Dark) {
        // Catppuccin Mocha
        pal.setColor(QPalette::Window,          base());
        pal.setColor(QPalette::WindowText,      text());
        pal.setColor(QPalette::Base,            mantle());
        pal.setColor(QPalette::AlternateBase,   QColor(0x24, 0x24, 0x37)); // #242437
        pal.setColor(QPalette::ToolTipBase,     surface1());
        pal.setColor(QPalette::ToolTipText,     text());
        pal.setColor(QPalette::Text,            text());
        pal.setColor(QPalette::Button,          surface0());
        pal.setColor(QPalette::ButtonText,      text());
        pal.setColor(QPalette::Link,            blue());
        pal.setColor(QPalette::Highlight,       surface1());
        pal.setColor(QPalette::HighlightedText, text());
    }
    else if (mode == ThemeMode::Light) {
        // Catppuccin Latte
        pal.setColor(QPalette::Window,          QColor(0xef, 0xf1, 0xf5)); // Latte Base
        pal.setColor(QPalette::WindowText,      QColor(0x4c, 0x4f, 0x69)); // Latte Text
        pal.setColor(QPalette::Base,            QColor(0xe6, 0xe9, 0xef)); // Latte Mantle
        pal.setColor(QPalette::AlternateBase,   QColor(0xdc, 0xe0, 0xe8)); // Latte Crust
        pal.setColor(QPalette::ToolTipBase,     QColor(0xcc, 0xd0, 0xda)); // Latte Surface0
        pal.setColor(QPalette::ToolTipText,     QColor(0x4c, 0x4f, 0x69));
        pal.setColor(QPalette::Text,            QColor(0x4c, 0x4f, 0x69));
        pal.setColor(QPalette::Button,          QColor(0xcc, 0xd0, 0xda));
        pal.setColor(QPalette::ButtonText,      QColor(0x4c, 0x4f, 0x69));
        pal.setColor(QPalette::Link,            QColor(0x1e, 0x66, 0xf5)); // Latte Blue
        pal.setColor(QPalette::Highlight,       QColor(0xac, 0xb0, 0xbe)); // Latte Surface1
        pal.setColor(QPalette::HighlightedText, QColor(0x4c, 0x4f, 0x69));
    }
    else {
        pal = QApplication::style()->standardPalette();
    }

    QApplication::setPalette(pal);
}

ThemeMode ThemePalette::current() { return currentMode_; }

// ═══════════════════════════════════════════════════════════════
//  Catppuccin Mocha Base Palette
// ═══════════════════════════════════════════════════════════════
QColor ThemePalette::crust()    {
    return currentMode_ == ThemeMode::Light ? QColor(0xdc, 0xe0, 0xe8) : QColor(0x11, 0x11, 0x1b);
}
QColor ThemePalette::mantle()   {
    return currentMode_ == ThemeMode::Light ? QColor(0xe6, 0xe9, 0xef) : QColor(0x18, 0x18, 0x25);
}
QColor ThemePalette::base()     {
    return currentMode_ == ThemeMode::Light ? QColor(0xef, 0xf1, 0xf5) : QColor(0x1e, 0x1e, 0x2e);
}
QColor ThemePalette::surface0() {
    return currentMode_ == ThemeMode::Light ? QColor(0xcc, 0xd0, 0xda) : QColor(0x31, 0x32, 0x44);
}
QColor ThemePalette::surface1() {
    return currentMode_ == ThemeMode::Light ? QColor(0xbc, 0xc0, 0xcc) : QColor(0x45, 0x47, 0x5a);
}
QColor ThemePalette::surface2() {
    return currentMode_ == ThemeMode::Light ? QColor(0xac, 0xb0, 0xbe) : QColor(0x58, 0x5b, 0x70);
}

// Text hierarchy
QColor ThemePalette::overlay0() {
    return currentMode_ == ThemeMode::Light ? QColor(0x9c, 0xa0, 0xb0) : QColor(0x6c, 0x70, 0x86);
}
QColor ThemePalette::overlay1() {
    return currentMode_ == ThemeMode::Light ? QColor(0x8c, 0x8f, 0xa1) : QColor(0x7f, 0x84, 0x9c);
}
QColor ThemePalette::subtext0() {
    return currentMode_ == ThemeMode::Light ? QColor(0x6c, 0x6f, 0x85) : QColor(0xa6, 0xad, 0xc8);
}
QColor ThemePalette::subtext1() {
    return currentMode_ == ThemeMode::Light ? QColor(0x5c, 0x5f, 0x77) : QColor(0xba, 0xc2, 0xde);
}
QColor ThemePalette::text()     {
    return currentMode_ == ThemeMode::Light ? QColor(0x4c, 0x4f, 0x69) : QColor(0xcd, 0xd6, 0xf4);
}

// ═══════════════════════════════════════════════════════════════
//  Accent Colors (same in both themes for consistency)
// ═══════════════════════════════════════════════════════════════
QColor ThemePalette::rosewater() { return QColor(0xf5, 0xe0, 0xdc); }
QColor ThemePalette::flamingo()  { return QColor(0xf2, 0xcd, 0xcd); }
QColor ThemePalette::pink()      { return QColor(0xf5, 0xc2, 0xe7); }
QColor ThemePalette::mauve()     { return QColor(0xcb, 0xa6, 0xf7); }
QColor ThemePalette::red()       { return QColor(0xf3, 0x8b, 0xa8); }
QColor ThemePalette::maroon()    { return QColor(0xeb, 0xa0, 0xac); }
QColor ThemePalette::peach()     { return QColor(0xfa, 0xb3, 0x87); }
QColor ThemePalette::yellow()    { return QColor(0xf9, 0xe2, 0xaf); }
QColor ThemePalette::green()     { return QColor(0xa6, 0xe3, 0xa1); }
QColor ThemePalette::teal()      { return QColor(0x94, 0xe2, 0xd5); }
QColor ThemePalette::sky()       { return QColor(0x89, 0xdc, 0xeb); }
QColor ThemePalette::sapphire()  { return QColor(0x74, 0xc7, 0xec); }
QColor ThemePalette::blue()      { return QColor(0x89, 0xb4, 0xfa); }
QColor ThemePalette::lavender()  { return QColor(0xb4, 0xbe, 0xfe); }

// ═══════════════════════════════════════════════════════════════
//  Semantic Aliases (backward-compatible)
// ═══════════════════════════════════════════════════════════════

// Chart colors — Catppuccin accents tuned for chart readability
QColor ThemePalette::chartBenign() { return green(); }
QColor ThemePalette::chartAttack() { return red(); }
QColor ThemePalette::chartTcp()    { return green(); }       // #a6e3a1
QColor ThemePalette::chartUdp()    { return yellow(); }      // #f9e2af
QColor ThemePalette::chartIcmp()   { return red(); }         // #f38ba8
QColor ThemePalette::chartPps()    { return sapphire(); }    // #74c7ec
QColor ThemePalette::chartOther()  { return subtext0(); }    // #a6adc8

// UI semantic
QColor ThemePalette::background()     { return crust(); }
QColor ThemePalette::cardBackground() { return base(); }
QColor ThemePalette::textPrimary()    { return text(); }
QColor ThemePalette::textSecondary()  { return subtext0(); }
QColor ThemePalette::accent()         { return green(); }
QColor ThemePalette::danger()         { return red(); }
QColor ThemePalette::warning()        { return peach(); }
QColor ThemePalette::success()        { return green(); }

// Heatmap
QColor ThemePalette::heatmapLow()  { QColor c = blue();   c.setAlpha(50);  return c; }
QColor ThemePalette::heatmapMid()  { QColor c = yellow(); c.setAlpha(150); return c; }
QColor ThemePalette::heatmapHigh() { QColor c = red();    c.setAlpha(220); return c; }

std::vector<QColor> ThemePalette::chartPalette() {
    return { chartTcp(), chartUdp(), chartIcmp(), chartPps(),
             chartBenign(), chartAttack(), mauve(), teal(), sky() };
}

// ═══════════════════════════════════════════════════════════════
//  QSS Helpers — centralized stylesheets
// ═══════════════════════════════════════════════════════════════

QString ThemePalette::cardStyleSheet() {
    return QString(
        "QFrame {"
        "  background: %1;"
        "  border: 1px solid %2;"
        "  border-radius: 8px;"
        "}")
        .arg(base().name(), surface0().name());
}

QString ThemePalette::tableStyleSheet() {
    return QString(
        "QTableWidget {"
        "  background-color: %1;"
        "  alternate-background-color: #242437;"
        "  color: %2;"
        "  gridline-color: transparent;"
        "  border: 1px solid %3;"
        "  border-radius: 8px;"
        "  padding: 5px;"
        "}"
        "QHeaderView::section {"
        "  background-color: %4;"
        "  color: %5;"
        "  font-weight: bold;"
        "  padding: 8px;"
        "  border: none;"
        "  text-transform: uppercase;"
        "  font-size: 10px;"
        "}"
        "QTableWidget::item {"
        "  padding: 10px;"
        "  border-bottom: 1px solid %3;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: %6;"
        "  color: %7;"
        "}")
        .arg(base().name(), text().name(), surface0().name(),
             mantle().name(), subtext0().name(),
             surface1().name(), rosewater().name());
}

QString ThemePalette::buttonStyleSheet() {
    return QString(
        "QPushButton {"
        "  background: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 4px;"
        "  padding: 8px 18px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: %3;"
        "}")
        .arg(surface0().name(), text().name(), surface1().name());
}
