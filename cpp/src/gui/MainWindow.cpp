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
#include "LoadingPopup.h"
#include "../audio/AudioEngine.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <QCoreApplication>
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
#include <QFileDialog>
#include <QFileInfo>
#include "CustomMessageBox.h"
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

    connect(&m_dirtyCheckTimer, &QTimer::timeout, this, &MainWindow::checkProjectDirty);
    m_dirtyCheckTimer.start(500); // Check dirty state every 500ms
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
    m_titleBar = new CustomTitleBar(this, true, true);
    mainLayout->addWidget(m_titleBar);

    // (Toolbar removed - Audio Settings moved to Settings menu in title bar)

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
    btnStandard->setFixedHeight(24);
    btnStandard->setFocusPolicy(Qt::NoFocus);
    m_armModeGroup->addButton(btnStandard, (int)ArmMode::Standard);
    
    QPushButton* btnUnion = new QPushButton("UNION");
    btnUnion->setCheckable(true);
    btnUnion->setFixedHeight(24);
    btnUnion->setFocusPolicy(Qt::NoFocus);
    m_armModeGroup->addButton(btnUnion, (int)ArmMode::Union);
    
    QPushButton* btnExclusive = new QPushButton("EXCLUSIVE");
    btnExclusive->setCheckable(true);
    btnExclusive->setFixedHeight(24);
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
    tcpList->setObjectName("TcpListWidget");
    tcpList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
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
    
    // Add Track Button
    QPushButton* addTrackBtn = new QPushButton("+ Add Track");
    addTrackBtn->setStyleSheet(
        "QPushButton { background: #333333; color: white; border: none; padding: 10px; font-weight: bold; font-size: 14px; }"
        "QPushButton:hover { background: #444444; }"
        "QPushButton:pressed { background: #222222; }"
    );
    tcpLayout->addWidget(addTrackBtn);
    connect(addTrackBtn, &QPushButton::clicked, this, &MainWindow::addTrack);


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
    
    m_btnRecord = new QPushButton(QChar(0x25CF), this); // Record circle
    m_btnRecord->setFixedSize(32, 24);
    m_btnRecord->setStyleSheet("color: #ff3333; font-size: 14px;");
    m_btnRecord->setFocusPolicy(Qt::NoFocus);
    connect(m_btnRecord, &QPushButton::clicked, this, &MainWindow::toggleRecording);

    transportLayout->addWidget(m_btnRecord);
    transportLayout->addWidget(m_btnPlayPause);
    transportLayout->addWidget(btnStop);
    transportLayout->addStretch();
    timelineLayout->addLayout(transportLayout);
    
    // Global spacebar shortcut for play/pause
    QShortcut* spaceShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    spaceShortcut->setContext(Qt::ApplicationShortcut);
    spaceShortcut->setAutoRepeat(false);
    connect(spaceShortcut, &QShortcut::activated, this, &MainWindow::togglePlayback);
    
    // Zoom shortcuts
    QShortcut* zoomInShortcut1 = new QShortcut(QKeySequence("Ctrl+="), this);
    zoomInShortcut1->setContext(Qt::ApplicationShortcut);
    connect(zoomInShortcut1, &QShortcut::activated, this, &MainWindow::zoomIn);
    
    QShortcut* zoomInShortcut2 = new QShortcut(QKeySequence("Ctrl++"), this);
    zoomInShortcut2->setContext(Qt::ApplicationShortcut);
    connect(zoomInShortcut2, &QShortcut::activated, this, &MainWindow::zoomIn);
    
    QShortcut* zoomOutShortcut = new QShortcut(QKeySequence("Ctrl+-"), this);
    zoomOutShortcut->setContext(Qt::ApplicationShortcut);
    connect(zoomOutShortcut, &QShortcut::activated, this, &MainWindow::zoomOut);
    
    // Add Track shortcut
    QShortcut* addTrackShortcut = new QShortcut(QKeySequence("Ctrl+T"), this);
    addTrackShortcut->setContext(Qt::ApplicationShortcut);
    connect(addTrackShortcut, &QShortcut::activated, this, &MainWindow::addTrack);
    
    // Record shortcut
    QShortcut* recordShortcut = new QShortcut(QKeySequence("R"), this);
    recordShortcut->setContext(Qt::ApplicationShortcut);
    recordShortcut->setAutoRepeat(false);
    connect(recordShortcut, &QShortcut::activated, this, &MainWindow::toggleRecording);
    
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
    // Menus live inside the custom title bar (not a separate QMenuBar)
    QMenuBar* menuBar = m_titleBar->menuBar();

    // ── File ──────────────────────────────────────────────────────────────────
    QMenu* fileMenu = menuBar->addMenu("FILE");

    QAction* newAction = fileMenu->addAction("New Project");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::newProject);

    QAction* openAction = fileMenu->addAction("Open Project...");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openProject);

    QAction* saveAction = fileMenu->addAction("Save Project");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveProject);

    QAction* saveAsAction = fileMenu->addAction("Save Project As...");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveProjectAs);

    fileMenu->addSeparator();

    QAction* exitAction = fileMenu->addAction("Exit");
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

    // ── Audio ─────────────────────────────────────────────────────────────────
    QMenu* audioMenu = menuBar->addMenu("AUDIO");

    QAction* playAction = audioMenu->addAction("Toggle Play / Stop");
    connect(playAction, &QAction::triggered, [this]() {
        if (m_engine) {
            bool isPlaying = m_engine->isEnginePlaying();
            m_engine->setPlaying(!isPlaying);
        }
    });

    // ── Settings ──────────────────────────────────────────────────────────────
    QMenu* settingsMenu = menuBar->addMenu("SETTINGS");

    QAction* audioSettingsAction = settingsMenu->addAction("Audio Settings...");
    connect(audioSettingsAction, &QAction::triggered, this, &MainWindow::openAudioSettings);
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
        // Because WM_NCCALCSIZE returns 0 (folding the frame into client area),
        // DefWindowProc will only return HTCLIENT for edge pixels. We must detect
        // resize zones ourselves by checking proximity to the window edges.
        POINT pt;
        pt.x = GET_X_LPARAM(msg->lParam);
        pt.y = GET_Y_LPARAM(msg->lParam);

        if (!isMaximized()) {
            RECT winRect;
            GetWindowRect(msg->hwnd, &winRect);

            const int border = 6; // resize grip width in screen pixels
            bool onLeft   = pt.x < winRect.left   + border;
            bool onRight  = pt.x >= winRect.right  - border;
            bool onTop    = pt.y < winRect.top     + border;
            bool onBottom = pt.y >= winRect.bottom - border;

            if (onLeft  && onTop)    { *result = HTTOPLEFT;     return true; }
            if (onRight && onTop)    { *result = HTTOPRIGHT;    return true; }
            if (onLeft  && onBottom) { *result = HTBOTTOMLEFT;  return true; }
            if (onRight && onBottom) { *result = HTBOTTOMRIGHT; return true; }
            if (onLeft)              { *result = HTLEFT;        return true; }
            if (onRight)             { *result = HTRIGHT;       return true; }
            if (onTop)               { *result = HTTOP;         return true; }
            if (onBottom)            { *result = HTBOTTOM;      return true; }
        }

        // Check if cursor is over the CustomTitleBar
        if (m_titleBar) {
            double dpi = devicePixelRatioF();
            QPoint globalPos(pt.x / dpi, pt.y / dpi);
            QPoint localPos = m_titleBar->mapFromGlobal(globalPos);

            if (m_titleBar->rect().contains(localPos)) {
                // Window control buttons → let Qt handle
                if (m_titleBar->isOverButton(globalPos)) {
                    return false;
                }
                // Embedded menu bar → let Qt handle so menus open
                QMenuBar* mb = m_titleBar->menuBar();
                if (mb) {
                    QPoint mbLocal = mb->mapFromGlobal(globalPos);
                    if (mb->rect().contains(mbLocal)) {
                        return false;
                    }
                }
                // Bare title area → caption drag
                *result = HTCAPTION;
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
        if (m_engine) {
            m_deviceManager->removeAudioCallback(m_engine);
        }
        
        gui::AudioSettingsDialog dialog(*m_deviceManager, this);
        dialog.exec();
        
        if (m_engine) {
            m_deviceManager->addAudioCallback(m_engine);
        }
    }
}

void MainWindow::reorderTracks(int fromIndex, int toIndex)
{
    if (!m_engine) return;
    
    dsp::EngineMessage msg;
    msg.type = dsp::EngineCommandType::MoveTrack;
    msg.trackIndex = fromIndex;
    msg.pluginIndex1 = toIndex; // Hack to use this field for toIndex
    m_engine->sendMessageFromUI(msg);
    
    // Reorder our local cache
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

void MainWindow::addTrack()
{
    if (!m_engine) return;
    
    // Find the tcpList widget
    QListWidget* tcpList = m_tcpPanel->findChild<QListWidget*>();
    if (!tcpList) return;
    
    int newTrackIndex = m_trackCards.size();
    QString trackName = QString("Track %1").arg(newTrackIndex + 1);
    
    // Send to backend
    dsp::EngineMessage msg;
    msg.type = dsp::EngineCommandType::AddTrack;
    std::strncpy(msg.stringValue, trackName.toStdString().c_str(), sizeof(msg.stringValue) - 1);
    m_engine->sendMessageFromUI(msg);
    
    // Add to UI
    QListWidgetItem* item = new QListWidgetItem(tcpList);
    item->setSizeHint(QSize(0, 100)); // TrackCard height
    
    TrackCard* card = new TrackCard(newTrackIndex, trackName, m_engine, tcpList);
    tcpList->setItemWidget(item, card);
    m_trackCards.push_back(card);
    connect(card, &TrackCard::clicked, this, &MainWindow::selectTrack);
    
    // Notify timeline to redraw
    auto* timeline = findChild<TimelineContainer*>();
    if (timeline) {
        timeline->update();
    }
}

void MainWindow::togglePlayback()
{
    if (m_engine) {
        m_isPlaying = !m_isPlaying;
        m_engine->setPlaying(m_isPlaying);
        
        if (!m_isPlaying) {
            m_btnPlayPause->setIcon(QIcon(":/icons/play.svg"));
            // Also stop recording if we stop playback
            if (m_isRecording) {
                m_isRecording = false;
                m_engine->setRecording(false);
                m_btnRecord->setStyleSheet("color: #ff3333; background-color: transparent; font-size: 14px;");
            }
        } else {
            m_btnPlayPause->setIcon(QIcon(":/icons/pause.svg"));
        }
    }
}

void MainWindow::toggleRecording()
{
    if (m_engine) {
        m_isRecording = !m_isRecording;
        m_engine->setRecording(m_isRecording);
        
        if (!m_isRecording) {
            // Stopped recording. Engine continues playing.
            m_btnRecord->setStyleSheet("color: #ff3333; background-color: transparent; font-size: 14px;");
            // Keep playing state as is, but ensure UI reflects it
            m_isPlaying = m_engine->isEnginePlaying();
            if (m_isPlaying) {
                m_btnPlayPause->setIcon(QIcon(":/icons/pause.svg"));
            } else {
                m_btnPlayPause->setIcon(QIcon(":/icons/play.svg"));
            }
        } else {
            // Started recording. Engine automatically plays.
            m_btnRecord->setStyleSheet("color: #ffffff; background-color: #ff0000; border-radius: 4px; font-size: 14px;");
            m_isPlaying = true;
            m_btnPlayPause->setIcon(QIcon(":/icons/pause.svg"));
        }
    }
}

void MainWindow::zoomIn()
{
    if (m_timeline) {
        // Zoom centering on the middle of the viewport
        QPoint center(m_timeline->viewport()->width() / 2, 0);
        m_timeline->zoom(1.2, center);
    }
}

void MainWindow::zoomOut()
{
    if (m_timeline) {
        // Zoom centering on the middle of the viewport
        QPoint center(m_timeline->viewport()->width() / 2, 0);
        m_timeline->zoom(1.0 / 1.2, center);
    }
}

void MainWindow::newProject()
{
    if (!promptSaveIfDirty()) return;
    
    if (!m_engine) return;
    m_engine->clearProject();
    m_currentProjectPath.clear();
    m_titleBar->setProjectName("");
    rebuildTrackUI();
}

void MainWindow::openProject()
{
    if (!promptSaveIfDirty()) return;

    QString fileName = QFileDialog::getOpenFileName(this, "Open Project", "", "Graphite Projects (*.graphite)");
    if (fileName.isEmpty()) return;
    
    if (!m_engine) return;
    
    juce::File file(fileName.toStdString());
    if (!file.existsAsFile()) return;

    juce::String xmlString = file.loadFileAsString();
    std::unique_ptr<juce::XmlElement> xml = juce::XmlDocument::parse(xmlString);
    if (xml == nullptr) {
        CustomMessageBox::critical(this, "Error", "Failed to parse project file.");
        return;
    }

    juce::ValueTree tree = juce::ValueTree::fromXml(*xml);
    
    // Stop audio device before loading - this is the ONLY correct approach.
    // JUCE's VST3 createPluginInstance() needs the MessageManager for dispatching.
    // With the audio callback removed, there is zero thread contention.
    if (m_deviceManager)
        m_deviceManager->removeAudioCallback(m_engine);

    // Show the loading popup (purely visual, indeterminate spinner)
    gui::LoadingPopup loadingPopup("LOADING PROJECT...", this);
    loadingPopup.show();
    QCoreApplication::processEvents();

    // Safe to call synchronously on main thread — no audio thread is running
    m_engine->deserializeProjectState(tree, nullptr);

    // Restart audio
    if (m_deviceManager)
        m_deviceManager->addAudioCallback(m_engine);

    loadingPopup.close();
    m_currentProjectPath = fileName;
    // Show the project name (file name without extension) in title bar
    m_titleBar->setProjectName(QFileInfo(fileName).baseName());
    rebuildTrackUI();
}

bool MainWindow::saveProject()
{
    if (m_currentProjectPath.isEmpty()) {
        return saveProjectAs();
    }
    
    if (!m_engine) return false;
    
    juce::ValueTree tree = m_engine->serializeProjectState();
    std::unique_ptr<juce::XmlElement> xml(tree.createXml());
    if (xml != nullptr) {
        juce::String xmlString = xml->createDocument(juce::String());
        juce::File file(m_currentProjectPath.toStdString());
        file.replaceWithText(xmlString);
        
        m_engine->clearProjectDirty();
        checkProjectDirty(); // update title immediately
        return true;
    }
    return false;
}

bool MainWindow::saveProjectAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save Project As", "", "Graphite Projects (*.graphite)");
    if (fileName.isEmpty()) return false;
    
    m_currentProjectPath = fileName;
    m_titleBar->setProjectName(QFileInfo(fileName).baseName());
    return saveProject();
}

void MainWindow::rebuildTrackUI()
{
    if (!m_engine) return;
    
    auto tracks = m_engine->getTracksSnapshot();
    
    // 1. Rebuild TCP
    QListWidget* tcpList = findChild<QListWidget*>("TcpListWidget");
    if (tcpList) {
        m_trackCards.clear();
        tcpList->clear();
        
        for (size_t i = 0; i < tracks.size(); ++i) {
            QListWidgetItem* item = new QListWidgetItem(tcpList);
            item->setSizeHint(QSize(0, 100)); // TrackCard height
            
            TrackCard* card = new TrackCard(tracks[i].id, QString::fromStdString(tracks[i].name), m_engine, tcpList);
            tcpList->setItemWidget(item, card);
            m_trackCards.push_back(card);
            connect(card, &TrackCard::clicked, this, &MainWindow::selectTrack);
        }
    }
    
    // 2. Rebuild Mixer
    MixerPanel* mixerTab = findChild<MixerPanel*>();
    if (mixerTab) {
        mixerTab->rebuildStrips(tracks);
        m_mixerStrips = mixerTab->getMixerStrips();
        
        for (auto* strip : m_mixerStrips) {
            if (strip->getTrackIndex() != -1) {
                connect(strip, &MixerStrip::clicked, this, &MainWindow::selectTrack);
            }
        }
    }
    
    // 3. Update timeline visually
    if (m_timeline) {
        m_timeline->update();
    }
    
    selectTrack(0);
}

void MainWindow::checkProjectDirty()
{
    if (!m_engine) return;
    bool isDirty = m_engine->isProjectDirty();
    if (isDirty != m_lastKnownDirty) {
        m_lastKnownDirty = isDirty;
        
        QString baseName = m_currentProjectPath.isEmpty() ? "" : QFileInfo(m_currentProjectPath).baseName();
        if (isDirty) {
            m_titleBar->setProjectName(baseName.isEmpty() ? "Unsaved Project*" : baseName + "*");
        } else {
            m_titleBar->setProjectName(baseName);
        }
    }
}

bool MainWindow::promptSaveIfDirty()
{
    if (m_engine && m_engine->isProjectDirty()) {
        QMessageBox::StandardButton reply;
        reply = CustomMessageBox::question(this, "Unsaved Changes",
            "The current project has unsaved changes. Do you want to save them?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            
        if (reply == QMessageBox::Yes) {
            return saveProject(); 
        } else if (reply == QMessageBox::Cancel) {
            return false;
        }
    }
    return true;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (promptSaveIfDirty()) {
        event->accept();
    } else {
        event->ignore();
    }
}

} // namespace gui
