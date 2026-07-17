#include "CustomTitleBar.h"
#include <QMainWindow>
#include <QHBoxLayout>
#include <QFont>
#include <QCursor>

namespace gui {

CustomTitleBar::CustomTitleBar(QMainWindow* parentWindow, const QString& titleText,
                               bool canMaximize, bool canMinimize)
    : QWidget(parentWindow), m_parentWindow(parentWindow)
{
    setFixedHeight(30);
    setObjectName("CustomTitleBar");
    
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(15, 0, 0, 0);
    layout->setSpacing(0);
    
    // Window Title label
    m_titleLabel = new QLabel(titleText.toUpper(), this);
    QFont font("Consolas", 9, QFont::Bold);
    m_titleLabel->setFont(font);
    m_titleLabel->setStyleSheet("color: #ffffff; background: transparent; letter-spacing: 1.0px;");
    layout->addWidget(m_titleLabel);
    
    layout->addStretch();
    
    // Minimize button
    if (canMinimize) {
        m_btnMin = new QPushButton("—", this);
        m_btnMin->setObjectName("TitleMinBtn");
        m_btnMin->setFixedSize(30, 30);
        m_btnMin->setCursor(Qt::PointingHandCursor);
        connect(m_btnMin, &QPushButton::clicked, m_parentWindow, &QMainWindow::showMinimized);
        layout->addWidget(m_btnMin);
    }
    
    // Maximize button
    if (canMaximize) {
        m_btnMax = new QPushButton(QChar(0x25A2), this); // White Square
        m_btnMax->setObjectName("TitleMaxBtn");
        m_btnMax->setFixedSize(30, 30);
        m_btnMax->setCursor(Qt::PointingHandCursor);
        connect(m_btnMax, &QPushButton::clicked, this, &CustomTitleBar::toggleMaximize);
        layout->addWidget(m_btnMax);
    }
    
    // Close button
    m_btnClose = new QPushButton("X", this);
    m_btnClose->setObjectName("TitleCloseBtn");
    m_btnClose->setFixedSize(30, 30);
    m_btnClose->setCursor(Qt::PointingHandCursor);
    connect(m_btnClose, &QPushButton::clicked, m_parentWindow, &QMainWindow::close);
    layout->addWidget(m_btnClose);
    
    // Apply styling specific to the TitleBar
    setStyleSheet(R"(
        QWidget#CustomTitleBar {
            background-color: #000000;
            border-bottom: 1px solid #222225;
        }
        QPushButton {
            background: transparent;
            border: none;
            color: #88888c;
            font-family: "Consolas", monospace;
            font-size: 11px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #ffffff;
            color: #000000;
        }
        QPushButton#TitleCloseBtn:hover {
            background-color: #ff0033;
            color: #ffffff;
        }
    )");
}

void CustomTitleBar::setTitle(const QString& title)
{
    m_titleLabel->setText(title.toUpper());
}

void CustomTitleBar::toggleMaximize()
{
    if (m_parentWindow->isMaximized()) {
        m_parentWindow->showNormal();
        if (m_btnMax) m_btnMax->setText(QString(QChar(0x25A2)));
    } else {
        m_parentWindow->showMaximized();
        if (m_btnMax) m_btnMax->setText(QString(QChar(0x29C9))); // Two joined squares
    }
}

bool CustomTitleBar::isOverButton(const QPoint& globalPos) const
{
    QPoint localPos = mapFromGlobal(globalPos);
    if (!rect().contains(localPos)) return false;
    
    if (m_btnClose && m_btnClose->geometry().contains(localPos)) return true;
    if (m_btnMin && m_btnMin->geometry().contains(localPos)) return true;
    if (m_btnMax && m_btnMax->geometry().contains(localPos)) return true;
    
    return false;
}

} // namespace gui
