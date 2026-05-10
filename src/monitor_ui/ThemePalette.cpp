#include "monitor_ui/ThemePalette.hpp"
#include <QStyle>

ThemeMode ThemePalette::currentMode_ = ThemeMode::Dark;

void ThemePalette::apply(ThemeMode mode) {
    currentMode_ = mode;

    QPalette pal;
    if (mode == ThemeMode::Dark) {
        pal.setColor(QPalette::Window, QColor(33, 33, 33));
        pal.setColor(QPalette::WindowText, QColor(210, 210, 210));
        pal.setColor(QPalette::Base, QColor(24, 24, 24));
        pal.setColor(QPalette::AlternateBase, QColor(33, 33, 33));
        pal.setColor(QPalette::ToolTipBase, QColor(45, 45, 45));
        pal.setColor(QPalette::ToolTipText, Qt::white);
        pal.setColor(QPalette::Text, QColor(210, 210, 210));
        pal.setColor(QPalette::Button, QColor(50, 50, 50));
        pal.setColor(QPalette::ButtonText, Qt::white);
        pal.setColor(QPalette::Link, QColor(137, 180, 250));
        pal.setColor(QPalette::Highlight, QColor(60, 60, 60));
        pal.setColor(QPalette::HighlightedText, Qt::white);
    }
    else if (mode == ThemeMode::Light) {
        pal.setColor(QPalette::Window, QColor(240, 240, 240));
        pal.setColor(QPalette::WindowText, Qt::black);
        pal.setColor(QPalette::Base, Qt::white);
        pal.setColor(QPalette::AlternateBase, QColor(240, 240, 240));
        pal.setColor(QPalette::ToolTipBase, Qt::white);
        pal.setColor(QPalette::ToolTipText, Qt::black);
        pal.setColor(QPalette::Text, Qt::black);
        pal.setColor(QPalette::Button, Qt::white);
        pal.setColor(QPalette::ButtonText, Qt::black);
        pal.setColor(QPalette::Link, QColor(0, 0, 238));
        pal.setColor(QPalette::Highlight, QColor(0, 120, 215));
        pal.setColor(QPalette::HighlightedText, Qt::white);
    }
    else {
        pal = QApplication::style()->standardPalette();
    }

    QApplication::setPalette(pal);
}

ThemeMode ThemePalette::current() { return currentMode_; }

QColor ThemePalette::chartBenign() { return QColor(100, 220, 100); }
QColor ThemePalette::chartAttack() { return QColor(255, 100, 100); }
QColor ThemePalette::chartTcp()    { return QColor(100, 150, 255); }
QColor ThemePalette::chartUdp()    { return QColor(255, 200, 100); }
QColor ThemePalette::chartIcmp()   { return QColor(200, 100, 255); }
QColor ThemePalette::chartPps()    { return QColor(100, 200, 255); }

QColor ThemePalette::background() {
    return currentMode_ == ThemeMode::Light ? QColor(240, 240, 240) : QColor(33, 33, 33);
}
QColor ThemePalette::cardBackground() {
    return currentMode_ == ThemeMode::Light ? Qt::white : QColor(44, 44, 44);
}
QColor ThemePalette::textPrimary() {
    return currentMode_ == ThemeMode::Light ? Qt::black : QColor(230, 230, 230);
}
QColor ThemePalette::textSecondary() {
    return currentMode_ == ThemeMode::Light ? QColor(100, 100, 100) : QColor(160, 160, 160);
}
QColor ThemePalette::accent()  { return QColor(100, 220, 100); }
QColor ThemePalette::danger()  { return QColor(255, 100, 100); }
QColor ThemePalette::warning() { return QColor(255, 200, 100); }
QColor ThemePalette::success() { return QColor(100, 220, 100); }

QColor ThemePalette::heatmapLow()  { return QColor(100, 150, 255, 50); }
QColor ThemePalette::heatmapMid()  { return QColor(255, 200, 100, 150); }
QColor ThemePalette::heatmapHigh() { return QColor(255, 100, 100, 220); }

std::vector<QColor> ThemePalette::chartPalette() {
    return {chartTcp(), chartUdp(), chartIcmp(), chartPps(),
            chartBenign(), chartAttack()};
}
