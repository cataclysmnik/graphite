#include "StyleTheme.h"
#include <QFontDatabase>

namespace gui {
namespace StyleTheme {

QString getGlobalStyleSheet()
{
    return R"(
        /* Global Backgrounds and Text */
        QWidget {
            background-color: #000000;
            color: #ffffff;
            font-family: "Consolas", monospace;
            font-size: 11px;
        }
        
        QMainWindow {
            background-color: #000000;
        }

        /* QSplitter */
        QSplitter::handle {
            background-color: #222225;
        }
        QSplitter::handle:horizontal {
            width: 2px;
        }
        QSplitter::handle:vertical {
            height: 2px;
        }

        /* Push Buttons (Default flat) */
        QPushButton {
            background-color: #0b0b0c;
            border: 1px solid #222225;
            border-radius: 4px;
            color: #88888c;
            padding: 4px 10px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #1a1a1c;
            border-color: #444448;
            color: #ffffff;
        }
        QPushButton:pressed {
            background-color: #ff0033;
            border-color: #ff0033;
            color: #ffffff;
        }
        QPushButton:checked {
            background-color: #ff0033;
            border-color: #ff0033;
            color: #ffffff;
        }

        /* Menu Bar */
        QMenuBar {
            background-color: #000000;
            border-bottom: 1px solid #222225;
            color: #ffffff;
        }
        QMenuBar::item {
            background-color: transparent;
            padding: 4px 10px;
        }
        QMenuBar::item:selected {
            background-color: #1a1a1c;
        }
        QMenu {
            background-color: #0b0b0c;
            border: 1px solid #222225;
            color: #ffffff;
        }
        QMenu::item:selected {
            background-color: #1a1a1c;
            color: #ff0033;
        }

        /* Tabs */
        QTabWidget::pane {
            border: 1px solid #222225;
            background-color: #000000;
        }
        QTabBar::tab {
            background-color: #0b0b0c;
            color: #88888c;
            border: 1px solid #222225;
            padding: 6px 12px;
            font-weight: bold;
            margin-right: -1px;
        }
        QTabBar::tab:selected {
            background-color: #1a1a1c;
            color: #ffffff;
            border-bottom: 2px solid #ff0033;
        }
        QTabBar::tab:hover:!selected {
            background-color: #1a1a1c;
            color: #ffffff;
        }
        
        /* ScrollBars */
        QScrollBar:vertical {
            background: #000000;
            width: 12px;
            margin: 0px 0px 0px 0px;
        }
        QScrollBar::handle:vertical {
            background: #222225;
            min-height: 20px;
            border-radius: 6px;
            margin: 2px;
        }
        QScrollBar::handle:vertical:hover {
            background: #444448;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        
        QScrollBar:horizontal {
            background: #000000;
            height: 12px;
            margin: 0px 0px 0px 0px;
        }
        QScrollBar::handle:horizontal {
            background: #222225;
            min-width: 20px;
            border-radius: 6px;
            margin: 2px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #444448;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
    )";
}

void applyTheme(QApplication& app)
{
    // Try to load Consolas or a suitable monospace font if available globally
    QFont defaultFont("Consolas", 9);
    defaultFont.setStyleHint(QFont::Monospace);
    app.setFont(defaultFont);
    
    app.setStyleSheet(getGlobalStyleSheet());
}

} // namespace StyleTheme
} // namespace gui
