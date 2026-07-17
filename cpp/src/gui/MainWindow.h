#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QTabWidget>

namespace dsp {
    class AudioEngine;
}

namespace gui {

class CustomTitleBar;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(dsp::AudioEngine* engine, QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
#ifdef _WIN32
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
#endif

private:
    void setupUi();
    void setupMenus();
    void enforceDarkImmersiveMode();

    dsp::AudioEngine* m_engine;
    
    // Core Layout Widgets
    CustomTitleBar* m_titleBar;
    QSplitter* m_mainSplitter;
    QSplitter* m_topWorkspace;
    QWidget* m_tcpPanel;
    QWidget* m_timelinePlaceholder;
    QTabWidget* m_bottomDock;
};

} // namespace gui
