#include "MainWindow.h"
#include "CustomTitleBar.h"
#include "../audio/AudioEngine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QLabel>
#include <QMenu>
#include <QAction>
#include <QCoreApplication>

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#endif

namespace gui {

MainWindow::MainWindow(dsp::AudioEngine* engine, QWidget* parent)
    : QMainWindow(parent), m_engine(engine)
{
    setWindowTitle("Graphite");
    setMinimumSize(950, 600);
    setObjectName("MainWindow");

#ifdef _WIN32
    // On Windows, use frameless window hint so we can draw custom titlebar, 
    // but we will intercept NCCALCSIZE to keep native shadow and resizing.
#else
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
#endif

    setupUi();
    setupMenus();

    // After layout is setup, force Windows DWM to dark mode
    enforceDarkImmersiveMode();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setObjectName("CentralWidget");
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Custom Title Bar
    m_titleBar = new CustomTitleBar(this, "GRAPHITE", true, true);
    mainLayout->addWidget(m_titleBar);

    // Main layout scaffolding
    m_mainSplitter = new QSplitter(Qt::Vertical, centralWidget);
    m_mainSplitter->setObjectName("MainVerticalSplitter");

    m_topWorkspace = new QSplitter(Qt::Horizontal, m_mainSplitter);
    m_topWorkspace->setObjectName("TopWorkspaceSplitter");

    // Left Panel (TCP)
    m_tcpPanel = new QWidget();
    m_tcpPanel->setObjectName("TcpPanel");
    QVBoxLayout* tcpLayout = new QVBoxLayout(m_tcpPanel);
    tcpLayout->setContentsMargins(10, 10, 10, 10);
    QLabel* tcpLabel = new QLabel("TRACKS (TBD)", m_tcpPanel);
    tcpLabel->setAlignment(Qt::AlignCenter);
    tcpLabel->setStyleSheet("color: #88888c; border: 1px dashed #444448;");
    tcpLayout->addWidget(tcpLabel);

    // Right Panel (Timeline)
    m_timelinePlaceholder = new QWidget();
    m_timelinePlaceholder->setObjectName("TimelinePlaceholder");
    QVBoxLayout* timelineLayout = new QVBoxLayout(m_timelinePlaceholder);
    timelineLayout->setContentsMargins(10, 10, 10, 10);
    QLabel* timelineLabel = new QLabel("TIMELINE (TBD)", m_timelinePlaceholder);
    timelineLabel->setAlignment(Qt::AlignCenter);
    timelineLabel->setStyleSheet("color: #88888c; border: 1px dashed #444448;");
    timelineLayout->addWidget(timelineLabel);

    m_topWorkspace->addWidget(m_tcpPanel);
    m_topWorkspace->addWidget(m_timelinePlaceholder);
    m_topWorkspace->setSizes({320, 680});
    m_mainSplitter->addWidget(m_topWorkspace);

    // Bottom Dock (Mixer, Signal Flow, etc)
    m_bottomDock = new QTabWidget(m_mainSplitter);
    m_bottomDock->setObjectName("BottomDockTabs");
    
    QWidget* mixerTab = new QWidget();
    QVBoxLayout* mixerLayout = new QVBoxLayout(mixerTab);
    QLabel* mixerLabel = new QLabel("MIXER (TBD)", mixerTab);
    mixerLabel->setAlignment(Qt::AlignCenter);
    mixerLabel->setStyleSheet("color: #88888c;");
    mixerLayout->addWidget(mixerLabel);
    
    QWidget* signalTab = new QWidget();
    
    m_bottomDock->addTab(mixerTab, "MIXER");
    m_bottomDock->addTab(signalTab, "SIGNAL FLOW");

    m_mainSplitter->addWidget(m_bottomDock);
    m_mainSplitter->setSizes({600, 250});

    mainLayout->addWidget(m_mainSplitter);
}

void MainWindow::setupMenus()
{
    QMenuBar* menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    QMenu* fileMenu = menuBar->addMenu("File");
    QAction* exitAction = fileMenu->addAction("Exit");
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

    QMenu* audioMenu = menuBar->addMenu("Audio");
    QAction* playAction = audioMenu->addAction("Toggle Play / Stop");
    connect(playAction, &QAction::triggered, [this]() {
        if (m_engine) {
            bool isPlaying = m_engine->isEnginePlaying();
            m_engine->setPlaying(!isPlaying);
        }
    });
}

void MainWindow::enforceDarkImmersiveMode()
{
#ifdef _WIN32
    HWND hwnd = reinterpret_cast<HWND>(winId());
    BOOL useDark = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDark, sizeof(useDark));

    // Force frame recalculation
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
#endif
}

#ifdef _WIN32
bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
    MSG* msg = static_cast<MSG*>(message);

    // Extend client area to window size (hides native titlebar, keeps shadows & resizing)
    if (msg->message == WM_NCCALCSIZE && msg->wParam == TRUE) {
        // Return 0 to indicate we handle the client area completely (no standard titlebar)
        *result = 0;
        return true;
    }

    if (msg->message == WM_NCHITTEST) {
        LRESULT hitTest = DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
        
        // If cursor is over standard resize borders (handled by DWM), return native hit tests
        if (hitTest == HTLEFT || hitTest == HTRIGHT || hitTest == HTTOP || hitTest == HTBOTTOM ||
            hitTest == HTTOPLEFT || hitTest == HTTOPRIGHT || hitTest == HTBOTTOMLEFT || hitTest == HTBOTTOMRIGHT) 
        {
            *result = hitTest;
            return true;
        }
        
        // Otherwise, check if cursor is over our CustomTitleBar
        POINT pt;
        pt.x = GET_X_LPARAM(msg->lParam);
        pt.y = GET_Y_LPARAM(msg->lParam);
        
        if (m_titleBar) {
            double dpi = devicePixelRatioF();
            QPoint globalPos(pt.x / dpi, pt.y / dpi);
            QPoint localPos = m_titleBar->mapFromGlobal(globalPos);
            
            if (m_titleBar->rect().contains(localPos)) {
                if (m_titleBar->isOverButton(globalPos)) {
                    // Let Qt handle the mouse event for the buttons
                    return false;
                }
                *result = HTCAPTION; // Allow dragging the window
                return true;
            }
        }
    }

    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif

} // namespace gui
