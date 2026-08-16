#include <QApplication>
#include <QIcon>
#include <QSplashScreen>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QFont>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <iostream>
#include "audio/AudioEngine.h"
#include "gui/MainWindow.h"
#include "gui/StyleTheme.h"

#include <QScreen>
#include <QGuiApplication>

class CustomSplashScreen : public QSplashScreen {
public:
    CustomSplashScreen(const QPixmap& originalPixmap) : QSplashScreen() {
        setWindowFlags(Qt::FramelessWindowHint | Qt::SplashScreen);
        setAttribute(Qt::WA_TranslucentBackground, true);

        if (!originalPixmap.isNull()) {
            this->pixmap = originalPixmap.scaledToWidth(600, Qt::SmoothTransformation);
            resize(this->pixmap.width(), this->pixmap.height() + 45);
        } else {
            resize(600, 445);
        }

        if (auto screen = QGuiApplication::primaryScreen()) {
            QRect screenGeom = screen->geometry();
            int x = (screenGeom.width() - width()) / 2;
            int y = (screenGeom.height() - height()) / 2;
            move(x, y);
        }
    }

    void setStatus(const QString& message, int progress) {
        this->message = message;
        this->progress = progress;
        update();
        QApplication::processEvents();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // 1. Background container
        QColor bgColor("#141415");
        painter.setBrush(QBrush(bgColor));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(rect(), 8, 8);

        // 2. Draw splashscreen.png
        if (!pixmap.isNull()) {
            painter.drawPixmap(0, 0, pixmap.width(), pixmap.height(), pixmap);
        }

        // Draw border
        QPen borderPen(QColor("#2d2d30"), 1);
        painter.setPen(borderPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 8, 8);

        // 3. Progress bar line
        int bar_h = 3;
        int bar_x = 40;
        int bar_w = width() - 80;
        int bar_y = height() - 35;

        // Inactive bar
        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(QColor("#2c2c2e")));
        painter.drawRoundedRect(bar_x, bar_y, bar_w, bar_h, 1.5, 1.5);

        // Active loading bar
        int progress_w = (int)(bar_w * (progress / 100.0f));
        if (progress_w > 0) {
            QLinearGradient active_gradient(bar_x, bar_y, bar_x + progress_w, bar_y);
            active_gradient.setColorAt(0, QColor("#8e8e93"));
            active_gradient.setColorAt(1, QColor("#ffffff"));
            painter.setBrush(QBrush(active_gradient));
            painter.drawRoundedRect(bar_x, bar_y, progress_w, bar_h, 1.5, 1.5);
        }

        // 3. Status Text
        QFont font_status("Segoe UI", 9);
        painter.setFont(font_status);
        painter.setPen(QColor("#8e8e93"));
        painter.drawText(QRect(bar_x, bar_y + 10, bar_w, 20), Qt::AlignLeft, message);
    }

private:
    QPixmap pixmap;
    QString message = "Initializing...";
    int progress = 0;
};

int main(int argc, char* argv[])
{
    // Initialize JUCE System
    juce::ScopedJuceInitialiser_GUI juceSystem;

    // Initialize Qt Application
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false); // We handle exiting through the MainWindow

    app.setWindowIcon(QIcon(":/logo.png"));

    QPixmap splashPixmap(":/splashscreen.png");
    CustomSplashScreen splash(splashPixmap);
    splash.show();
    splash.setStatus("Applying themes...", 10);

    // Apply the Graphite Dark Theme
    gui::StyleTheme::applyTheme(app);

    splash.setStatus("Initializing audio engine...", 30);
    // Initialize our C++ Audio Engine
    dsp::AudioEngine engine;

    // Setup JUCE Audio Device Manager
    juce::AudioDeviceManager deviceManager;
    
    // Determine Settings File Location: %APPDATA%/GuitarDaw/AudioSettings.xml
    juce::File appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("GuitarDaw");
    if (!appDataDir.exists()) {
        appDataDir.createDirectory();
    }
    juce::File audioSettingsFile = appDataDir.getChildFile("AudioSettings.xml");
    
    splash.setStatus("Loading audio settings...", 50);
    // Attempt to load settings
    std::unique_ptr<juce::XmlElement> savedAudioState = nullptr;
    if (audioSettingsFile.existsAsFile()) {
        savedAudioState = juce::XmlDocument::parse(audioSettingsFile);
    }
    
    splash.setStatus("Starting audio devices...", 70);
    juce::String err;
    if (savedAudioState != nullptr) {
        err = deviceManager.initialise(2, 2, savedAudioState.get(), true);
    } else {
        err = deviceManager.initialiseWithDefaultDevices(2, 2);
    }
    
    // Fallback if loading failed or returned error
    if (err.isNotEmpty() && savedAudioState != nullptr) {
        std::cout << "Error loading saved audio settings: " << err.toStdString() << ". Reverting to default.\n";
        err = deviceManager.initialiseWithDefaultDevices(2, 2);
    }
    
    if (err.isNotEmpty()) {
        std::cout << "Error initializing default audio device: " << err.toStdString() << "\n";
    }

    if (auto* device = deviceManager.getCurrentAudioDevice()) {
        std::cout << "JUCE Default Audio Device: " << device->getName().toStdString() << "\n";
    }

    // Hook our audio engine into the soundcard
    deviceManager.addAudioCallback(&engine);

    splash.setStatus("Creating main interface...", 90);
    // Create the Main Window
    gui::MainWindow window(&engine, &deviceManager);
    window.showMaximized();

    splash.setStatus("Ready.", 100);
    splash.finish(&window);

    int result = app.exec();

    // Clean up
    deviceManager.removeAudioCallback(&engine);
    
    // Save audio settings on exit
    std::unique_ptr<juce::XmlElement> audioState(deviceManager.createStateXml());
    if (audioState != nullptr) {
        audioState->writeTo(audioSettingsFile, juce::XmlElement::TextFormat());
    }

    return result;
}
