#include "monitor_ui/ThemePalette.hpp"
#include <QStyle>

ThemeMode ThemePalette::currentMode_ = ThemeMode::Dark;

void ThemePalette::apply(ThemeMode mode) {
    currentMode_ = mode;

    QPalette pal;
    if (mode == ThemeMode::Dark) {
        pal.setColor(QPalette::Window, QColor(30, 30, 46));
        pal.setColor(QPalette::WindowText, QColor(205, 214, 244));
        pal.setColor(QPalette::Base, QColor(24, 24, 37));
        pal.setColor(QPalette::AlternateBase, QColor(30, 30, 46));
        pal.setColor(QPalette::ToolTipBase, QColor(49, 50, 68));
        pal.setColor(QPalette::ToolTipText, QColor(205, 214, 244));
        pal.setColor(QPalette::Text, QColor(205, 214, 244));
        pal.setColor(QPalette::Button, QColor(49, 50, 68));
        pal.setColor(QPalette::ButtonText, QColor(205, 214, 244));
        pal.setColor(QPalette::Link, QColor(137, 180, 250));
        pal.setColor(QPalette::Highlight, QColor(137, 180, 250));
        pal.setColor(QPalette::HighlightedText, QColor(30, 30, 46));
    }
    else if (mode == ThemeMode::Light) {
        pal.setColor(QPalette::Window, QColor(239, 241, 245));
        pal.setColor(QPalette::WindowText, QColor(76, 79, 105));
        pal.setColor(QPalette::Base, QColor(255, 255, 255));
        pal.setColor(QPalette::AlternateBase, QColor(239, 241, 245));
        pal.setColor(QPalette::ToolTipBase, QColor(204, 208, 218));
        pal.setColor(QPalette::ToolTipText, QColor(76, 79, 105));
        pal.setColor(QPalette::Text, QColor(76, 79, 105));
        pal.setColor(QPalette::Button, QColor(204, 208, 218));
        pal.setColor(QPalette::ButtonText, QColor(76, 79, 105));
        pal.setColor(QPalette::Link, QColor(30, 102, 245));
        pal.setColor(QPalette::Highlight, QColor(30, 102, 245));
        pal.setColor(QPalette::HighlightedText, Qt::white);
    }
    else {
        // System default
        pal = QApplication::style()->standardPalette();
    }

    QApplication::setPalette(pal);
}

ThemeMode ThemePalette::current() { return currentMode_; }

QColor ThemePalette::chartBenign() {
    return currentMode_ == ThemeMode::Light ? QColor(64, 160, 43) : QColor(166, 227, 161);
}
QColor ThemePalette::chartAttack() {
    return currentMode_ == ThemeMode::Light ? QColor(210, 15, 57) : QColor(243, 139, 168);
}
QColor ThemePalette::chartTcp() {
    return currentMode_ == ThemeMode::Light ? QColor(30, 102, 245) : QColor(137, 180, 250);
}
QColor ThemePalette::chartUdp() {
    return currentMode_ == ThemeMode::Light ? QColor(223, 142, 29) : QColor(249, 226, 175);
}
QColor ThemePalette::chartIcmp() {
    return currentMode_ == ThemeMode::Light ? QColor(136, 57, 239) : QColor(203, 166, 247);
}
QColor ThemePalette::chartPps() {
    return currentMode_ == ThemeMode::Light ? QColor(4, 165, 229) : QColor(116, 199, 236);
}

QColor ThemePalette::background() {
    return currentMode_ == ThemeMode::Light ? QColor(239, 241, 245) : QColor(30, 30, 46);
}
QColor ThemePalette::cardBackground() {
    return currentMode_ == ThemeMode::Light ? QColor(255, 255, 255) : QColor(49, 50, 68);
}
QColor ThemePalette::textPrimary() {
    return currentMode_ == ThemeMode::Light ? QColor(76, 79, 105) : QColor(205, 214, 244);
}
QColor ThemePalette::textSecondary() {
    return currentMode_ == ThemeMode::Light ? QColor(124, 127, 147) : QColor(166, 173, 200);
}
QColor ThemePalette::accent() {
    return currentMode_ == ThemeMode::Light ? QColor(30, 102, 245) : QColor(137, 180, 250);
}
QColor ThemePalette::danger() {
    return currentMode_ == ThemeMode::Light ? QColor(210, 15, 57) : QColor(243, 139, 168);
}
QColor ThemePalette::warning() {
    return currentMode_ == ThemeMode::Light ? QColor(223, 142, 29) : QColor(249, 226, 175);
}
QColor ThemePalette::success() {
    return currentMode_ == ThemeMode::Light ? QColor(64, 160, 43) : QColor(166, 227, 161);
}

QColor ThemePalette::heatmapLow()  { return QColor(30, 102, 245, 40); }
QColor ThemePalette::heatmapMid()  { return QColor(249, 226, 175, 128); }
QColor ThemePalette::heatmapHigh() { return QColor(243, 139, 168, 220); }

std::vector<QColor> ThemePalette::chartPalette() {
    return {chartTcp(), chartUdp(), chartIcmp(), chartPps(),
            chartBenign(), chartAttack()};
}
