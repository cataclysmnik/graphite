#include "CustomTitleBar.h"
#include <QMainWindow>
#include <QHBoxLayout>
#include <QFont>

namespace gui {

CustomTitleBar::CustomTitleBar(QMainWindow* parentWindow,
                               bool canMaximize, bool canMinimize)
    : QWidget(parentWindow), m_parentWindow(parentWindow)
{
    setFixedHeight(34);
    setObjectName("CustomTitleBar");

    // Use a horizontal layout with three zones: [menus | center title | win buttons]
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ── LEFT: Embedded menu bar ───────────────────────────────────────────────
    m_menuBar = new QMenuBar(this);
    m_menuBar->setObjectName("TitleMenuBar");
    m_menuBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    layout->addWidget(m_menuBar, 0, Qt::AlignVCenter);

    // ── CENTER: "GRAPHITE  ·  project name" ──────────────────────────────────
    m_titleLabel = new QLabel("GRAPHITE", this);
    m_titleLabel->setObjectName("TitleLabel");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_titleLabel, 1, Qt::AlignCenter); // stretch = 1 → fills space

    // ── RIGHT: Window control buttons ─────────────────────────────────────────
    if (canMinimize) {
        m_btnMin = new QPushButton("—", this);
        m_btnMin->setObjectName("TitleMinBtn");
        m_btnMin->setFixedSize(34, 34);
        m_btnMin->setCursor(Qt::PointingHandCursor);
        m_btnMin->setFocusPolicy(Qt::NoFocus);
        connect(m_btnMin, &QPushButton::clicked, m_parentWindow, &QMainWindow::showMinimized);
        layout->addWidget(m_btnMin);
    }

    if (canMaximize) {
        m_btnMax = new QPushButton(QChar(0x25A2), this);
        m_btnMax->setObjectName("TitleMaxBtn");
        m_btnMax->setFixedSize(34, 34);
        m_btnMax->setCursor(Qt::PointingHandCursor);
        m_btnMax->setFocusPolicy(Qt::NoFocus);
        connect(m_btnMax, &QPushButton::clicked, this, &CustomTitleBar::toggleMaximize);
        layout->addWidget(m_btnMax);
    }

    m_btnClose = new QPushButton("✕", this);
    m_btnClose->setObjectName("TitleCloseBtn");
    m_btnClose->setFixedSize(34, 34);
    m_btnClose->setCursor(Qt::PointingHandCursor);
    m_btnClose->setFocusPolicy(Qt::NoFocus);
    connect(m_btnClose, &QPushButton::clicked, m_parentWindow, &QMainWindow::close);
    layout->addWidget(m_btnClose);

    setStyleSheet(R"(
        QWidget#CustomTitleBar {
            background-color: #000000;
            border-bottom: 1px solid #1a1a1c;
        }

        /* ── Embedded menu bar ── */
        QMenuBar#TitleMenuBar {
            background: transparent;
            color: #88888c;
            font-family: "Consolas", monospace;
            font-size: 10px;
            font-weight: bold;
            letter-spacing: 0.5px;
            padding-left: 8px;
            border: none;
        }
        QMenuBar#TitleMenuBar::item {
            background: transparent;
            padding: 4px 10px;
            color: #88888c;
        }
        QMenuBar#TitleMenuBar::item:selected,
        QMenuBar#TitleMenuBar::item:pressed {
            background-color: #1a1a1c;
            color: #ffffff;
        }
        QMenu {
            background-color: #0b0b0c;
            color: #cccccc;
            border: 1px solid #2a2a2d;
            font-family: "Consolas", monospace;
            font-size: 10px;
            font-weight: bold;
            padding: 4px 0px;
        }
        QMenu::item {
            padding: 6px 24px;
            color: #cccccc;
        }
        QMenu::item:selected {
            background-color: #1f1f22;
            color: #ffffff;
        }
        QMenu::item:disabled {
            color: #444446;
        }
        QMenu::separator {
            height: 1px;
            background: #2a2a2d;
            margin: 4px 0px;
        }

        /* ── Center title ── */
        QLabel#TitleLabel {
            color: #ffffff;
            background: transparent;
            font-family: "Consolas", monospace;
            font-size: 10px;
            font-weight: bold;
            letter-spacing: 1.5px;
        }

        /* ── Window control buttons ── */
        QPushButton#TitleMinBtn,
        QPushButton#TitleMaxBtn {
            background: transparent;
            border: none;
            color: #66666a;
            font-family: "Consolas", monospace;
            font-size: 11px;
        }
        QPushButton#TitleMinBtn:hover,
        QPushButton#TitleMaxBtn:hover {
            background-color: #1f1f22;
            color: #ffffff;
        }
        QPushButton#TitleCloseBtn {
            background: transparent;
            border: none;
            color: #66666a;
            font-size: 11px;
        }
        QPushButton#TitleCloseBtn:hover {
            background-color: #ff0033;
            color: #ffffff;
        }
    )");
}

void CustomTitleBar::setProjectName(const QString& projectName)
{
    if (projectName.isEmpty()) {
        m_titleLabel->setText("GRAPHITE");
    } else {
        // "GRAPHITE  ·  My Project"
        m_titleLabel->setText(QString("GRAPHITE  \u00B7  %1").arg(projectName.toUpper()));
    }
}

void CustomTitleBar::toggleMaximize()
{
    if (m_parentWindow->isMaximized()) {
        m_parentWindow->showNormal();
        if (m_btnMax) m_btnMax->setText(QString(QChar(0x25A2)));
    } else {
        m_parentWindow->showMaximized();
        if (m_btnMax) m_btnMax->setText(QString(QChar(0x25A3))); // filled square = "restore"
    }
}

bool CustomTitleBar::isOverButton(const QPoint& globalPos) const
{
    QPoint localPos = mapFromGlobal(globalPos);
    if (!rect().contains(localPos)) return false;

    if (m_btnClose && m_btnClose->geometry().contains(localPos)) return true;
    if (m_btnMin  && m_btnMin->geometry().contains(localPos))   return true;
    if (m_btnMax  && m_btnMax->geometry().contains(localPos))   return true;

    return false;
}

} // namespace gui
