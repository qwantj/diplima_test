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
        // Catppuccin Mocha — все фоны строго тёмные
        pal.setColor(QPalette::Window,          base());        // #1e1e2e
        pal.setColor(QPalette::WindowText,      text());        // #cdd6f4
        pal.setColor(QPalette::Base,            crust());       // #11111b — основной фон таблиц
        pal.setColor(QPalette::AlternateBase,   mantle());      // #181825 — чередование строк
        pal.setColor(QPalette::ToolTipBase,     surface1());
        pal.setColor(QPalette::ToolTipText,     text());
        pal.setColor(QPalette::Text,            text());
        pal.setColor(QPalette::Button,          surface0());
        pal.setColor(QPalette::ButtonText,      text());
        pal.setColor(QPalette::Link,            blue());
        // Мягкое выделение — не кричащее
        pal.setColor(QPalette::Highlight,       QColor(0x31, 0x32, 0x44)); // surface0
        pal.setColor(QPalette::HighlightedText, text());
        pal.setColor(QPalette::Mid,             surface0());
        pal.setColor(QPalette::Dark,            crust());
        pal.setColor(QPalette::Shadow,          QColor(0x09, 0x09, 0x12));
    }
    else if (mode == ThemeMode::Light) {
        // Улучшенная светлая тема (High Contrast Light)
        pal.setColor(QPalette::Window,          QColor(0xff, 0xff, 0xff)); // Pure White background
        pal.setColor(QPalette::WindowText,      QColor(0x00, 0x00, 0x00)); // Strictly Black text
        pal.setColor(QPalette::Base,            QColor(0xff, 0xff, 0xff)); // Pure White base
        pal.setColor(QPalette::AlternateBase,   QColor(0xf8, 0xf9, 0xfb)); // Very subtle alternate rows
        pal.setColor(QPalette::ToolTipBase,     QColor(0xff, 0xff, 0xff));
        pal.setColor(QPalette::ToolTipText,     QColor(0x00, 0x00, 0x00));
        pal.setColor(QPalette::Text,            QColor(0x00, 0x00, 0x00));
        pal.setColor(QPalette::Button,          QColor(0xef, 0xf1, 0xf5));
        pal.setColor(QPalette::ButtonText,      QColor(0x00, 0x00, 0x00));
        pal.setColor(QPalette::Link,            QColor(0x1e, 0x66, 0xf5));
        pal.setColor(QPalette::Highlight,       QColor(0xde, 0xe2, 0xe9)); // Subtle highlight
        pal.setColor(QPalette::HighlightedText, QColor(0x00, 0x00, 0x00));
        pal.setColor(QPalette::Mid,             QColor(0xcc, 0xd0, 0xda));
        pal.setColor(QPalette::Dark,            QColor(0xdc, 0xe0, 0xe8));
        pal.setColor(QPalette::Shadow,          QColor(0xbc, 0xc0, 0xcc));
    }
    else {
        pal = QApplication::style()->standardPalette();
    }

    QApplication::setPalette(pal);

    // Применяем глобальный QSS для полного покрытия виджетов
    qApp->setStyleSheet(globalStyleSheet());
}

ThemeMode ThemePalette::current() { return currentMode_; }

// ═══════════════════════════════════════════════════════════════
//  Catppuccin Mocha Base Palette
// ═══════════════════════════════════════════════════════════════
QColor ThemePalette::crust()    {
    return currentMode_ == ThemeMode::Light ? QColor(0xf1, 0xf3, 0xf6) : QColor(0x11, 0x11, 0x1b);
}
QColor ThemePalette::mantle()   {
    return currentMode_ == ThemeMode::Light ? QColor(0xf8, 0xf9, 0xfb) : QColor(0x18, 0x18, 0x25);
}
QColor ThemePalette::base()     {
    return currentMode_ == ThemeMode::Light ? QColor(0xff, 0xff, 0xff) : QColor(0x1e, 0x1e, 0x2e);
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
    return currentMode_ == ThemeMode::Light ? QColor(0x00, 0x00, 0x00) : QColor(0xa6, 0xad, 0xc8);
}
QColor ThemePalette::subtext1() {
    return currentMode_ == ThemeMode::Light ? QColor(0x00, 0x00, 0x00) : QColor(0xba, 0xc2, 0xde);
}
QColor ThemePalette::text()     {
    return currentMode_ == ThemeMode::Light ? QColor(0x00, 0x00, 0x00) : QColor(0xcd, 0xd6, 0xf4);
}

// ═══════════════════════════════════════════════════════════════
//  Accent Colors (Adaptive for Light/Dark contrast)
// ═══════════════════════════════════════════════════════════════
QColor ThemePalette::rosewater() { return QColor(0xf5, 0xe0, 0xdc); }
QColor ThemePalette::flamingo()  { return QColor(0xf2, 0xcd, 0xcd); }
QColor ThemePalette::pink()      { return QColor(0xf5, 0xc2, 0xe7); }
QColor ThemePalette::mauve()     { return QColor(0xcb, 0xa6, 0xf7); }
QColor ThemePalette::red()       { 
    return currentMode_ == ThemeMode::Light ? QColor(0xd2, 0x0f, 0x39) : QColor(0xf3, 0x8b, 0xa8); 
}
QColor ThemePalette::maroon()    { return QColor(0xeb, 0xa0, 0xac); }
QColor ThemePalette::peach()     { 
    return currentMode_ == ThemeMode::Light ? QColor(0xfe, 0x64, 0x33) : QColor(0xfa, 0xb3, 0x87); 
}
QColor ThemePalette::yellow()    { 
    return currentMode_ == ThemeMode::Light ? QColor(0xdf, 0x8e, 0x1d) : QColor(0xf9, 0xe2, 0xaf); 
}
QColor ThemePalette::green()     { 
    return currentMode_ == ThemeMode::Light ? QColor(0x40, 0xa0, 0x2b) : QColor(0xa6, 0xe3, 0xa1); 
}
QColor ThemePalette::teal()      { return QColor(0x94, 0xe2, 0xd5); }
QColor ThemePalette::sky()       { return QColor(0x89, 0xdc, 0xeb); }
QColor ThemePalette::sapphire()  { 
    return currentMode_ == ThemeMode::Light ? QColor(0x20, 0x9f, 0xb5) : QColor(0x74, 0xc7, 0xec); 
}
QColor ThemePalette::blue()      { 
    return currentMode_ == ThemeMode::Light ? QColor(0x1e, 0x66, 0xf5) : QColor(0x89, 0xb4, 0xfa); 
}
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
QColor ThemePalette::chartOther()  { return mauve(); }       // #cba6f7

// UI semantic
QColor ThemePalette::background()     { return crust(); }    // Самый тёмный фон
QColor ThemePalette::cardBackground() { return base(); }     // Фон карточек
QColor ThemePalette::textPrimary()    { return text(); }
QColor ThemePalette::textSecondary()  { return subtext0(); }
QColor ThemePalette::accent()         { return green(); }
QColor ThemePalette::danger()         { return red(); }
QColor ThemePalette::warning()        { return peach(); }
QColor ThemePalette::success()        { return green(); }

// Heatmap
QColor ThemePalette::heatmapLow()  { QColor c = blue();   c.setAlpha(80);  return c; }
QColor ThemePalette::heatmapMid()  { QColor c = yellow(); c.setAlpha(180); return c; }
QColor ThemePalette::heatmapHigh() { QColor c = red();    c.setAlpha(230); return c; }

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
    // Мягкое выделение — цвет строки чуть светлее фона, без кричащего контраста
    QString headerBg = currentMode_ == ThemeMode::Light ? QColor(0xf1, 0xf3, 0xf6).name() : mantle().name();
    QString itemBorder = currentMode_ == ThemeMode::Light ? QColor(0xef, 0xf1, 0xf5).name() : surface0().name();

    return QString(
        "QTableWidget {"
        "  background-color: %1;"
        "  alternate-background-color: %8;"
        "  color: %2;"
        "  gridline-color: transparent;"
        "  border: 1px solid %3;"
        "  border-radius: 8px;"
        "  padding: 2px;"
        "}"
        "QHeaderView::section {"
        "  background-color: %4;"
        "  color: %2;"
        "  font-weight: bold;"
        "  padding: 8px;"
        "  border: none;"
        "  text-transform: uppercase;"
        "  font-size: 10px;"
        "}"
        "QHeaderView::section:hover {"
        "  background-color: %3;"
        "}"
        "QTableWidget::item {"
        "  padding: 8px 10px;"
        "  border-bottom: 1px solid %9;"
        "  background: transparent;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: %6;"
        "  color: %2;"
        "  border: none;"
        "}"
        "QTableWidget::item:hover {"
        "  background-color: %7;"
        "}")
        .arg(crust().name(),        // %1 background
             text().name(),          // %2 color
             surface0().name(),      // %3 border/grid
             headerBg,               // %4 header bg
             subtext0().name(),      // %5 header color
             surface0().name(),      // %6 selected (мягкое)
             surface0().name(),      // %7 hover
             mantle().name(),        // %8 alternate
             itemBorder);            // %9 item border
}

QString ThemePalette::buttonStyleSheet() {
    return QString(
        "QPushButton {"
        "  background: %1;"
        "  color: %2;"
        "  border: 1px solid %3;"
        "  border-radius: 4px;"
        "  padding: 5px 14px;"
        "  font-weight: bold;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "  background: %3;"
        "  border-color: %4;"
        "}"
        "QPushButton:pressed {"
        "  background: %4;"
        "}")
        .arg(surface0().name(), text().name(), surface1().name(), surface2().name());
}

// ═══════════════════════════════════════════════════════════════
//  Global QSS — обеспечивает единый стиль для ВСЕХ виджетов
// ═══════════════════════════════════════════════════════════════
QString ThemePalette::globalStyleSheet() {
    QString bg    = crust().name();
    QString bg2   = base().name();
    QString bg3   = mantle().name();
    QString surf0 = surface0().name();
    QString surf1 = surface1().name();
    QString txt   = text().name();
    QString txt2  = subtext0().name();
    QString acc   = green().name();

    return QString(
        // === Базовые виджеты ===
        "QWidget {"
        "  background-color: %1;"
        "  color: %6;"
        "  font-family: 'Segoe UI', Arial, sans-serif;"
        "  font-size: 12px;"
        "}"

        // === Главное окно и QFrame ===
        "QMainWindow, QDialog {"
        "  background-color: %1;"
        "}"

        // === GroupBox ===
        "QGroupBox {"
        "  background-color: %2;"
        "  border: 1px solid %4;"
        "  border-radius: 8px;"
        "  margin-top: 8px;"
        "  padding-top: 6px;"
        "  font-weight: bold;"
        "  color: %7;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  left: 10px;"
        "  padding: 0 4px;"
        "  color: %7;"
        "}"

        // === ScrollArea ===
        "QScrollArea {"
        "  background: %1;"
        "  border: none;"
        "}"

        // === ScrollBar ===
        "QScrollBar:vertical {"
        "  background: %3;"
        "  width: 8px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: %4;"
        "  border-radius: 4px;"
        "  min-height: 20px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0;"
        "}"
        "QScrollBar:horizontal {"
        "  background: %3;"
        "  height: 8px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background: %4;"
        "  border-radius: 4px;"
        "  min-width: 20px;"
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "  width: 0;"
        "}"

        // === ToolBar ===
        "QToolBar {"
        "  background: %3;"
        "  border-bottom: 1px solid %4;"
        "  spacing: 8px;"
        "}"

        // === StatusBar ===
        "QStatusBar {"
        "  background: %3;"
        "  color: %7;"
        "  border-top: 1px solid %4;"
        "}"

        // === ComboBox ===
        "QComboBox {"
        "  background: %4;"
        "  color: %6;"
        "  border: 1px solid %5;"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "  min-width: 80px;"
        "}"
        "QComboBox:hover {"
        "  border-color: %8;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: %3;"
        "  color: %6;"
        "  border: 1px solid %4;"
        "  selection-background-color: %4;"
        "}"

        // === LineEdit / DateEdit ===
        "QLineEdit, QDateEdit, QSpinBox {"
        "  background: %4;"
        "  color: %6;"
        "  border: 1px solid %5;"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "}"
        "QLineEdit:focus, QDateEdit:focus {"
        "  border-color: %8;"
        "}"

        // === CheckBox ===
        "QCheckBox {"
        "  color: %6;"
        "  spacing: 6px;"
        "}"
        "QCheckBox::indicator {"
        "  width: 14px;"
        "  height: 14px;"
        "  border: 2px solid %5;"
        "  border-radius: 3px;"
        "  background: %4;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background: %8;"
        "  border-color: %8;"
        "}"

        // === PushButton ===
        "QPushButton {"
        "  background: %4;"
        "  color: %6;"
        "  border: 1px solid %5;"
        "  border-radius: 4px;"
        "  padding: 5px 14px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: %5;"
        "  border-color: %8;"
        "}"
        "QPushButton:pressed {"
        "  background: %5;"
        "}"

        // === Label (без фона по умолчанию) ===
        "QLabel {"
        "  background: transparent;"
        "  color: %6;"
        "}"

        // === ToolTip ===
        "QToolTip {"
        "  background: %3;"
        "  color: %6;"
        "  border: 1px solid %4;"
        "  padding: 4px;"
        "  border-radius: 4px;"
        "}"

        // === Menu ===
        "QMenu {"
        "  background: %3;"
        "  color: %6;"
        "  border: 1px solid %4;"
        "}"
        "QMenu::item:selected {"
        "  background: %4;"
        "}"

        // === TabWidget ===
        "QTabWidget::pane {"
        "  background: %2;"
        "  border: 1px solid %4;"
        "}"
        "QTabBar::tab {"
        "  background: %3;"
        "  color: %7;"
        "  padding: 6px 16px;"
        "  border-radius: 4px 4px 0 0;"
        "}"
        "QTabBar::tab:selected {"
        "  background: %2;"
        "  color: %6;"
        "}"

        // === Custom Classes ===
        "*[cssClass=\"sidebarList\"] {"
        "  background: %3; border: none; padding: 0px;"
        "  font-size: 13px; color: %12; outline: none;"
        "}"
        "*[cssClass=\"sidebarList\"]::item {"
        "  padding: 15px 18px; border-left: 4px solid transparent;"
        "}"
        "*[cssClass=\"sidebarList\"]::item:selected {"
        "  background: %2; color: %8; border-left: 4px solid %8;"
        "}"
        "*[cssClass=\"sidebarList\"]::item:hover:!selected {"
        "  background: %4;"
        "}"
    ) + tableStyleSheet() + QString(
        "*[cssClass=\"toolbarBtnYellow\"] {"
        "  padding: 5px 12px; color: %11; font-weight: bold; background: %4; border: 1px solid %5; border-radius: 4px;"
        "}"
        "*[cssClass=\"toolbarBtnYellow\"]:hover { background: %5; }"

        "*[cssClass=\"toolbarBtnGreen\"] {"
        "  padding: 5px 12px; color: %8; font-weight: bold; background: %4; border: 1px solid %5; border-radius: 4px;"
        "}"
        "*[cssClass=\"toolbarBtnGreen\"]:hover { background: %5; }"

        "*[cssClass=\"toolbarBtnPeach\"] {"
        "  padding: 5px 12px; color: %10; font-weight: bold; background: %4; border: 1px solid %5; border-radius: 4px;"
        "}"
        "*[cssClass=\"toolbarBtnPeach\"]:hover { background: %5; }"

        "*[cssClass=\"toolbarBtnRed\"] {"
        "  padding: 5px 12px; color: white; font-weight: bold; background: #d20f39; border: 1px solid #d20f39; border-radius: 4px;"
        "}"

        "*[cssClass=\"dashboardCard\"] {"
        "  background: %2;"
        "  border: 1px solid %4;"
        "  border-radius: 8px;"
        "}"

        "*[cssClass=\"overviewTitle\"] {"
        "  color: %6; font-size: 13px; font-weight: bold; background: transparent; padding: 5px;"
        "}"
        "*[cssClass=\"chartTitle\"] {"
        "  color: %6; font-size: 14px; font-weight: bold; border: none; background: transparent;"
        "}"

        "*[cssClass=\"donutTitle\"] {"
        "  color: %6; font-weight: bold; font-size: 14px; border: none; background: transparent;"
        "}"
        "*[cssClass=\"statusLabelGreen\"] {"
        "  color: %8; font-size: 12px; font-weight: bold; background: transparent;"
        "}"
        "*[cssClass=\"statusLabelRed\"] {"
        "  color: %9; font-size: 12px; font-weight: bold; background: transparent;"
        "}"
        "*[cssClass=\"statusLabelPeach\"] {"
        "  color: %10; font-size: 12px; font-weight: bold; background: transparent;"
        "}"
        "*[cssClass=\"statusLabelText\"] {"
        "  color: %6; font-size: 12px; font-weight: bold; background: transparent;"
        "}"

        "*[cssClass=\"dashboardCheckbox\"] {"
        "  color: %6; font-size: 12px; font-weight: bold; background: transparent;"
        "}"
    )
    .arg(bg,     // %1 crust
         bg2,    // %2 base
         bg3,    // %3 mantle
         surf0,  // %4 surface0
         surf1,  // %5 surface1
         txt,    // %6 text
         txt2,   // %7 subtext0
         acc,    // %8 green
         red().name(),   // %9 red
         peach().name(), // %10 peach
         yellow().name(),// %11 yellow
         subtext1().name() // %12 subtext1
    );
}
