#include "MainWindow.h"
#include "CustomTitleBar.h"
#include "TrackCard.h"
#include "Timeline.h"
#include "AudioSettingsDialog.h"
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
#include <QListWidget>
#include <QDropEvent>
#include <QMetaObject>
#include <QButtonGroup>
#include <QScrollBar>
#include <QKeyEvent>
#include <QShortcut>

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

    // Transport controls moved from here to above TCP
    
    toolbarLayout->addStretch();
    
    QPushButton* btnSettings = new QPushButton("Audio Settings...", toolbar);
    connect(btnSettings, &QPushButton::clicked, this, &MainWindow::openAudioSettings);
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
    tcpLayout->setContentsMargins(10, 4, 10, 4);
    tcpLayout->setSpacing(0);
    
    // Transport & Arm Mode Toolbar
    QHBoxLayout* armModeLayout = new QHBoxLayout();
    armModeLayout->setContentsMargins(0, 0, 0, 4);
    armModeLayout->setSpacing(6);
    
    // Arm Mode Toolbar (Transport moved to timeline panel)
    m_armModeGroup = new QButtonGroup(this);
    m_armModeGroup->setExclusive(true);
    
    QPushButton* btnStandard = new QPushButton("STANDARD");
    btnStandard->setCheckable(true);
    btnStandard->setChecked(true);
    btnStandard->setFixedHeight(20);
    btnStandard->setFocusPolicy(Qt::NoFocus);
    m_armModeGroup->addButton(btnStandard, (int)ArmMode::Standard);
    
    QPushButton* btnUnion = new QPushButton("UNION");
    btnUnion->setCheckable(true);
    btnUnion->setFixedHeight(20);
    btnUnion->setFocusPolicy(Qt::NoFocus);
    m_armModeGroup->addButton(btnUnion, (int)ArmMode::Union);
    
    QPushButton* btnExclusive = new QPushButton("EXCLUSIVE");
    btnExclusive->setCheckable(true);
    btnExclusive->setFixedHeight(20);
    btnExclusive->setFocusPolicy(Qt::NoFocus);
    m_armModeGroup->addButton(btnExclusive, (int)ArmMode::Exclusive);
    
    armModeLayout->addWidget(btnStandard);
    armModeLayout->addWidget(btnUnion);
    armModeLayout->addWidget(btnExclusive);
    
    tcpLayout->addLayout(armModeLayout);
    
    class TcpListWidget : public QListWidget {
    public:
        TcpListWidget(MainWindow* mainWindow, QWidget* parent = nullptr) 
            : QListWidget(parent), m_mainWindow(mainWindow) {
            setDragDropMode(QAbstractItemView::InternalMove);
            setSelectionMode(QAbstractItemView::SingleSelection);
            setFocusPolicy(Qt::NoFocus);
            setStyleSheet(
                "QListWidget { background: transparent; border: none; outline: 0; }"
                "QListWidget::item { padding: 0px; margin-bottom: 4px; }"
                "QListWidget::item:selected { background: transparent; border: none; }"
            );
        }
    protected:
        void dropEvent(QDropEvent* event) override {
            int from = currentRow();
            QListWidget::dropEvent(event);
            int to = currentRow();
            if (from != -1 && to != -1 && from != to) {
                QMetaObject::invokeMethod(m_mainWindow, "reorderTracks", Qt::QueuedConnection, Q_ARG(int, from), Q_ARG(int, to));
            }
        }
    private:
        MainWindow* m_mainWindow;
    };
    
    TcpListWidget* tcpList = new TcpListWidget(this, m_tcpPanel);
    tcpLayout->addWidget(tcpList);
    
    // Add default tracks
    const char* trackNames[] = {"Lead Guitar", "Rhythm Guitar", "Bass", "Drums"};
    for (int i = 0; i < 4; ++i) {
        QListWidgetItem* item = new QListWidgetItem(tcpList);
        item->setSizeHint(QSize(0, 100)); // TrackCard height
        
        TrackCard* card = new TrackCard(i, trackNames[i], m_engine, tcpList);
        tcpList->setItemWidget(item, card);
        m_trackCards.push_back(card);
        connect(card, &TrackCard::clicked, this, &MainWindow::selectTrack);
    }

    // Right Panel (Timeline)
    QWidget* timelinePanel = new QWidget();
    timelinePanel->setObjectName("TimelinePanel");
    QVBoxLayout* timelineLayout = new QVBoxLayout(timelinePanel);
    timelineLayout->setContentsMargins(10, 4, 10, 4);
    timelineLayout->setSpacing(0);
    
    // Transport Toolbar (above timeline)
    QHBoxLayout* transportLayout = new QHBoxLayout();
    transportLayout->setContentsMargins(0, 0, 0, 4);
    transportLayout->setSpacing(6);
    
    m_btnPlayPause = new QPushButton(QIcon(":/icons/play.svg"), "");
    m_btnPlayPause->setFixedSize(32, 24);
    m_btnPlayPause->setIconSize(QSize(16, 16));
    m_btnPlayPause->setFocusPolicy(Qt::NoFocus);
    connect(m_btnPlayPause, &QPushButton::clicked, this, &MainWindow::togglePlayback);
    
    QPushButton* btnStop = new QPushButton(QIcon(":/icons/stop.svg"), "");
    btnStop->setFixedSize(32, 24);
    btnStop->setIconSize(QSize(16, 16));
    btnStop->setFocusPolicy(Qt::NoFocus);
    connect(btnStop, &QPushButton::clicked, [this]() {
        if (m_engine) {
            m_engine->setPlaying(false);
            m_btnPlayPause->setIcon(QIcon(":/icons/play.svg"));
        }
    });
    
    transportLayout->addWidget(m_btnPlayPause);
    transportLayout->addWidget(btnStop);
    transportLayout->addStretch();
    timelineLayout->addLayout(transportLayout);
    
    // Global spacebar shortcut for play/pause
    QShortcut* spaceShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    spaceShortcut->setContext(Qt::ApplicationShortcut);
    connect(spaceShortcut, &QShortcut::activated, this, &MainWindow::togglePlayback);
    
    m_timeline = new TimelineContainer(m_engine, timelinePanel);
    m_timeline->setObjectName("TimelineContainer");
    timelineLayout->addWidget(m_timeline);
    
    // Synchronize vertical scrolling between TCP and Timeline
    connect(tcpList->verticalScrollBar(), &QScrollBar::valueChanged,
            m_timeline->verticalScrollBar(), &QScrollBar::setValue);
    connect(m_timeline->verticalScrollBar(), &QScrollBar::valueChanged,
            tcpList->verticalScrollBar(), &QScrollBar::setValue);

    m_topWorkspace->addWidget(m_tcpPanel);
    m_topWorkspace->addWidget(timelinePanel);
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
    
    QSplitter* effectsSplitter = new QSplitter(Qt::Horizontal, effectsTab);
    effectsSplitter->setObjectName("EffectsSplitter");
    
    m_effectsRack = new gui::EffectsRack(m_engine, effectsSplitter);
    
    // Auto-save audio settings on change
    class AudioSettingsSaver : public juce::ChangeListener {
    public:
        AudioSettingsSaver(juce::AudioDeviceManager* dm) : manager(dm) {
            manager->addChangeListener(this);
        }
        ~AudioSettingsSaver() {
            manager->removeChangeListener(this);
        }
        void changeListenerCallback(juce::ChangeBroadcaster*) override {
            juce::File appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("GuitarDaw");
            if (!appDataDir.exists()) appDataDir.createDirectory();
            juce::File audioSettingsFile = appDataDir.getChildFile("AudioSettings.xml");
            if (auto xml = manager->createStateXml()) {
                xml->writeTo(audioSettingsFile);
            }
        }
    private:
        juce::AudioDeviceManager* manager;
    };
    static AudioSettingsSaver settingsSaver(m_deviceManager);

    gui::SignalFlow* signalFlow = new gui::SignalFlow(m_engine, effectsSplitter);
    
    effectsSplitter->addWidget(m_effectsRack);
    effectsSplitter->addWidget(signalFlow);
    effectsSplitter->setSizes({500, 500}); // 50/50 split initially
    
    effectsLayout->addWidget(effectsSplitter);
    
    m_bottomDock->addTab(effectsTab, "EFFECTS");
    m_bottomDock->addTab(mixerTab, "MIXER");

    m_mainSplitter->addWidget(m_bottomDock);
    m_mainSplitter->setSizes({600, 250});

    mainLayout->addWidget(m_mainSplitter);

    // Action wiring now handled inline for transport
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
    
    // Apply arming logic based on mode
    if (index >= 0 && index < m_trackCards.size()) {
        if (m_armModeGroup->checkedId() == (int)ArmMode::Union) {
            bool currentArm = m_trackCards[index]->isArmed();
            m_trackCards[index]->setArmed(!currentArm);
        } else if (m_armModeGroup->checkedId() == (int)ArmMode::Exclusive) {
            for (int i = 0; i < m_trackCards.size(); ++i) {
                m_trackCards[i]->setArmed(i == index);
            }
        }
    }
    
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

void MainWindow::openAudioSettings()
{
    if (m_deviceManager) {
        gui::AudioSettingsDialog dialog(*m_deviceManager, this);
        dialog.exec();
    }
}

void MainWindow::reorderTracks(int fromIndex, int toIndex)
{
    if (!m_engine) return;
    
    m_engine->moveTrackSynchronous(fromIndex, toIndex);
    
    // Update local lists
    auto card = m_trackCards[fromIndex];
    m_trackCards.erase(m_trackCards.begin() + fromIndex);
    m_trackCards.insert(m_trackCards.begin() + toIndex, card);
    
    for (int i = 0; i < m_trackCards.size(); ++i) {
        m_trackCards[i]->setTrackIndex(i);
    }
    
    if (m_timeline) {
        m_timeline->update();
    }
    
    // Request MixerPanel to reorder
    if (m_bottomDock->count() > 1) {
        // MixerPanel is the second tab
        if (auto mixerPanel = qobject_cast<MixerPanel*>(m_bottomDock->widget(1))) {
            mixerPanel->reorderStrips(fromIndex, toIndex);
        }
    }
}

void MainWindow::togglePlayback()
{
    if (m_engine) {
        bool isPlaying = m_engine->isEnginePlaying();
        m_engine->setPlaying(!isPlaying);
        m_btnPlayPause->setIcon(QIcon(isPlaying ? ":/icons/play.svg" : ":/icons/pause.svg"));
    }
}

} // namespace gui
