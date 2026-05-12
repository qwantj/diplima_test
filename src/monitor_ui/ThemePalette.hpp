#pragma once

#include <QColor>
#include <QPalette>
#include <QApplication>
#include <vector>

enum class ThemeMode { System, Dark, Light };

Q_DECLARE_METATYPE(ThemeMode)

class ThemePalette {
public:
    static void apply(ThemeMode mode);
    static ThemeMode current();

    // ── Catppuccin Base Palette ──────────────────────────────
    // Surface layers (dark → light)
    static QColor crust();       // Darkest background
    static QColor mantle();      // Sidebar / header background
    static QColor base();        // Main content background
    static QColor surface0();    // Card borders, dividers
    static QColor surface1();    // Hover state, secondary cards
    static QColor surface2();    // Active state

    // Text hierarchy
    static QColor overlay0();    // Disabled / placeholder text
    static QColor overlay1();    // Subtle text
    static QColor subtext0();    // Secondary text (labels, captions)
    static QColor subtext1();    // Tertiary text
    static QColor text();        // Primary text

    // ── Accent Colors ────────────────────────────────────────
    static QColor rosewater();   // #f5e0dc
    static QColor flamingo();    // #f2cdcd
    static QColor pink();        // #f5c2e7
    static QColor mauve();       // #cba6f7
    static QColor red();         // #f38ba8
    static QColor maroon();      // #eba0ac
    static QColor peach();       // #fab387
    static QColor yellow();      // #f9e2af
    static QColor green();       // #a6e3a1
    static QColor teal();        // #94e2d5
    static QColor sky();         // #89dceb
    static QColor sapphire();    // #74c7ec
    static QColor blue();        // #89b4fa
    static QColor lavender();    // #b4befe

    // ── Semantic Aliases (backward-compatible) ───────────────
    // Chart colors
    static QColor chartBenign();
    static QColor chartAttack();
    static QColor chartTcp();
    static QColor chartUdp();
    static QColor chartIcmp();
    static QColor chartPps();
    static QColor chartOther();

    // UI semantic colors
    static QColor background();      // → crust()
    static QColor cardBackground();  // → base()
    static QColor textPrimary();     // → text()
    static QColor textSecondary();   // → subtext0()
    static QColor accent();          // → green()
    static QColor danger();          // → red()
    static QColor warning();         // → peach()
    static QColor success();         // → green()

    // Heatmap
    static QColor heatmapLow();
    static QColor heatmapMid();
    static QColor heatmapHigh();

    // Get gradient colors for charts
    static std::vector<QColor> chartPalette();

    // ── QSS Helpers ──────────────────────────────────────────
    // Returns a complete QSS stylesheet for consistent card styling
    static QString cardStyleSheet();
    // Returns QSS for styled tables (used in Analytics, Log, Incidents)
    static QString tableStyleSheet();
    // Returns QSS for styled buttons
    static QString buttonStyleSheet();

private:
    static ThemeMode currentMode_;
};
