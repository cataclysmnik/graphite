#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QMenuBar>

class QMainWindow;

namespace gui {

class CustomTitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit CustomTitleBar(QMainWindow* parentWindow,
                            bool canMaximize = true, bool canMinimize = true);

    // Updates the project name shown next to "GRAPHITE" in the center
    void setProjectName(const QString& projectName);

    // Expose the embedded menu bar so MainWindow can add menus to it
    QMenuBar* menuBar() const { return m_menuBar; }

    // Check if a point (in global coords) is over a window control button
    bool isOverButton(const QPoint& globalPos) const;

private slots:
    void toggleMaximize();

private:
    QMainWindow*  m_parentWindow;
    QMenuBar*     m_menuBar;
    QLabel*       m_titleLabel;
    QPushButton*  m_btnMin  {nullptr};
    QPushButton*  m_btnMax  {nullptr};
    QPushButton*  m_btnClose{nullptr};
};

} // namespace gui
