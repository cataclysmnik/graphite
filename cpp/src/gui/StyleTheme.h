#pragma once
#include <QApplication>
#include <QString>

namespace gui {
namespace StyleTheme {

    // Applies the global "Nothing" aesthetic stylesheet to the QApplication
    void applyTheme(QApplication& app);
    
    // Helper to get the raw stylesheet string if needed for specific widgets
    QString getGlobalStyleSheet();

} // namespace StyleTheme
} // namespace gui
