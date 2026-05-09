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

    // Chart colors
    static QColor chartBenign();
    static QColor chartAttack();
    static QColor chartTcp();
    static QColor chartUdp();
    static QColor chartIcmp();
    static QColor chartPps();

    // UI colors
    static QColor background();
    static QColor cardBackground();
    static QColor textPrimary();
    static QColor textSecondary();
    static QColor accent();
    static QColor danger();
    static QColor warning();
    static QColor success();

    // Heatmap
    static QColor heatmapLow();
    static QColor heatmapMid();
    static QColor heatmapHigh();

    // Get gradient colors for charts
    static std::vector<QColor> chartPalette();

private:
    static ThemeMode currentMode_;
};
