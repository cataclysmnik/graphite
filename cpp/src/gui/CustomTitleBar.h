#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>

class QMainWindow;

namespace gui {

class CustomTitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit CustomTitleBar(QMainWindow* parentWindow, const QString& titleText = "GRAPHITE",
                            bool canMaximize = true, bool canMinimize = true);

    void setTitle(const QString& title);
    
    // Check if a point (in global coords) is over a window control button
    bool isOverButton(const QPoint& globalPos) const;

private slots:
    void toggleMaximize();

private:
    QMainWindow* m_parentWindow;
    QLabel* m_titleLabel;
    QPushButton* m_btnMin {nullptr};
    QPushButton* m_btnMax {nullptr};
    QPushButton* m_btnClose {nullptr};
};

} // namespace gui
