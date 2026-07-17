#include "MainWindow.h"
#include "CustomTitleBar.h"
#include "TrackCard.h"
#include "Timeline.h"
#include "MixerPanel.h"
#include "MixerStrip.h"
#include "EffectsRack.h"
#include "SignalFlow.h"
#include "AudioSettingsDialog.h"
#include "../audio/AudioEngine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QLabel>
#include <QMenu>
#include <QAction>
#include <QCoreApplication>
#include <QScrollArea>
#include <QIcon>
#include <QPushButton>

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

MainWindow::MainWindow(dsp::AudioEngine* engine, juce::AudioDeviceManager* deviceManager, QWidget* parent)
    : QMainWindow(parent), m_engine(engine), m_deviceManager(deviceManager)
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

    // Toolbar
    QWidget* toolbar = new QWidget(centralWidget);
    toolbar->setObjectName("MainToolbar");
    toolbar->setFixedHeight(40);
    toolbar->setStyleSheet("QWidget#MainToolbar { background-color: #0b0b0c; border-bottom: 1px solid #222225; }");
    
    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(10, 4, 10, 4);
    toolbarLayout->setSpacing(8);

    QPushButton* btnPlay = new QPushButton(QIcon(":/icons/play.svg"), "", toolbar);
    QPushButton* btnPause = new QPushButton(QIcon(":/icons/pause.svg"), "", toolbar);
    QPushButton* btnStop = new QPushButton(QIcon(":/icons/stop.svg"), "", toolbar);
    QPushButton* btnRecord = new QPushButton(QIcon(":/icons/record.svg"), "", toolbar);
    
    // Style icons
    for (auto btn : {btnPlay, btnPause, btnStop, btnRecord}) {
        btn->setFixedSize(32, 32);
        btn->setIconSize(QSize(16, 16));
        toolbarLayout->addWidget(btn);
    }
    
    toolbarLayout->addStretch();
    
    QPushButton* btnSettings = new QPushButton("Audio Settings...", toolbar);
    toolbarLayout->addWidget(btnSettings);
    
    mainLayout->addWidget(toolbar);

    // Main layout scaffolding
    m_mainSplitter = new QSplitter(Qt::Vertical, centralWidget);
    m_mainSplitter->setObjectName("MainVerticalSplitter");

    m_topWorkspace = new QSplitter(Qt::Horizontal, m_mainSplitter);
    m_topWorkspace->setObjectName("TopWorkspaceSplitter");

    // Left Panel (TCP)
    m_tcpPanel = new QWidget();
    m_tcpPanel->setObjectName("TcpPanel");
    QVBoxLayout* tcpLayout = new QVBoxLayout(m_tcpPanel);
    tcpLayout->setContentsMargins(0, 0, 0, 0);
    tcpLayout->setSpacing(0);
    
    QScrollArea* tcpScroll = new QScrollArea();
    tcpScroll->setWidgetResizable(true);
    tcpScroll->setFrameShape(QFrame::NoFrame);
    
    QWidget* tcpContainer = new QWidget();
    QVBoxLayout* tcpContainerLayout = new QVBoxLayout(tcpContainer);
    tcpContainerLayout->setContentsMargins(5, 5, 5, 5);
    tcpContainerLayout->setSpacing(5);
    
    // Add default tracks
    const char* trackNames[] = {"Lead Guitar", "Rhythm Guitar", "Bass", "Drums"};
    for (int i = 0; i < 4; ++i) {
        TrackCard* card = new TrackCard(i, trackNames[i], m_engine, tcpContainer);
        tcpContainerLayout->addWidget(card);
        m_trackCards.push_back(card);
        connect(card, &TrackCard::clicked, this, &MainWindow::selectTrack);
    }
    tcpContainerLayout->addStretch();
    
    tcpScroll->setWidget(tcpContainer);
    tcpLayout->addWidget(tcpScroll);

    // Right Panel (Timeline)
    TimelineContainer* timeline = new TimelineContainer(m_engine, m_topWorkspace);
    timeline->setObjectName("TimelineContainer");

    m_topWorkspace->addWidget(m_tcpPanel);
    m_topWorkspace->addWidget(timeline);
    m_topWorkspace->setSizes({320, 680});
    m_mainSplitter->addWidget(m_topWorkspace);

    // Bottom Dock (Mixer, Signal Flow, etc)
    m_bottomDock = new QTabWidget(m_mainSplitter);
    m_bottomDock->setObjectName("BottomDockTabs");
    
    // Add dock toggle button
    QWidget* cornerWidget = new QWidget();
    QHBoxLayout* cornerLayout = new QHBoxLayout(cornerWidget);
    cornerLayout->setContentsMargins(0, 0, 4, 0);
    cornerLayout->setSpacing(6);
    
    QPushButton* btnDockToggle = new QPushButton("▼", cornerWidget);
    btnDockToggle->setFixedHeight(24);
    btnDockToggle->setToolTip("Collapse / Expand Panel");
    btnDockToggle->setCursor(Qt::PointingHandCursor);
    btnDockToggle->setStyleSheet("background-color: transparent; border: none; font-size: 10px; color: #88888c;");
    cornerLayout->addWidget(btnDockToggle);
    m_bottomDock->setCornerWidget(cornerWidget, Qt::TopRightCorner);
    
    // Connect dock toggle to simply collapse/expand the QSplitter
    connect(btnDockToggle, &QPushButton::clicked, [this, btnDockToggle]() {
        QList<int> sizes = m_mainSplitter->sizes();
        if (sizes[1] > 0) {
            // Collapse
            m_mainSplitter->setSizes({ sizes[0] + sizes[1], 0 });
            btnDockToggle->setText("▲");
        } else {
            // Expand to roughly 250px
            int h = m_mainSplitter->height();
            int newDockH = 250;
            m_mainSplitter->setSizes({ h - newDockH, newDockH });
            btnDockToggle->setText("▼");
        }
    });
    
    MixerPanel* mixerTab = new MixerPanel(m_engine, m_bottomDock);
    m_mixerStrips = mixerTab->getMixerStrips();
    for (auto* strip : m_mixerStrips) {
        connect(strip, &MixerStrip::clicked, this, &MainWindow::selectTrack);
    }
    
    QWidget* effectsTab = new QWidget();
    QHBoxLayout* effectsLayout = new QHBoxLayout(effectsTab);
    effectsLayout->setContentsMargins(0, 0, 0, 0);
    effectsLayout->setSpacing(0);
    
    m_effectsRack = new gui::EffectsRack(m_engine, effectsTab);
    m_effectsRack->setFixedWidth(250);
    
    gui::SignalFlow* signalFlow = new gui::SignalFlow(m_engine, effectsTab);
    
    effectsLayout->addWidget(m_effectsRack);
    effectsLayout->addWidget(signalFlow);
    
    m_bottomDock->addTab(mixerTab, "MIXER");
    m_bottomDock->addTab(effectsTab, "EFFECTS");

    m_mainSplitter->addWidget(m_bottomDock);
    m_mainSplitter->setSizes({600, 250});

    mainLayout->addWidget(m_mainSplitter);

    // Wire up actions
    connect(btnPlay, &QPushButton::clicked, [this]() {
        if (m_engine) m_engine->setPlaying(true);
    });
    connect(btnPause, &QPushButton::clicked, [this]() {
        if (m_engine) m_engine->setPlaying(false);
    });
    connect(btnSettings, &QPushButton::clicked, [this]() {
        if (m_deviceManager) {
            gui::AudioSettingsDialog dialog(*m_deviceManager, this);
            dialog.exec();
        }
    });
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

void MainWindow::selectTrack(int index)
{
    m_selectedTrackIndex = index;
    
    // Update TrackCards visually
    for (size_t i = 0; i < m_trackCards.size(); ++i) {
        m_trackCards[i]->setSelected(m_trackCards[i]->property("trackIndex").toInt() == index || (int)i == index);
    }
    
    // Update MixerStrips visually
    for (size_t i = 0; i < m_mixerStrips.size(); ++i) {
        // We know master is index -1, so handle safely (or just rely on the order/properties)
        // To be safe, just compare against the loop index assuming 0..3
        m_mixerStrips[i]->setSelected((int)i == index || m_mixerStrips[i]->property("trackIndex").toInt() == index);
    }
    
    // Send to AudioEngine
    if (m_engine) {
        dsp::EngineMessage msg;
        msg.type = dsp::EngineCommandType::SetTrackSelect;
        msg.trackIndex = index;
        m_engine->sendMessageFromUI(msg);
    }
    
    // Update EffectsRack
    if (m_effectsRack) {
        m_effectsRack->updateForTrack(index);
    }
}

} // namespace gui
