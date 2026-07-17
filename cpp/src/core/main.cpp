#include <QApplication>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <iostream>
#include "audio/AudioEngine.h"
#include "gui/MainWindow.h"
#include "gui/StyleTheme.h"

int main(int argc, char* argv[])
{
    // Initialize JUCE System
    juce::ScopedJuceInitialiser_GUI juceSystem;

    // Initialize Qt Application
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false); // We handle exiting through the MainWindow

    // Apply the Graphite Dark Theme
    gui::StyleTheme::applyTheme(app);

    // Initialize our C++ Audio Engine
    dsp::AudioEngine engine;

    // Setup JUCE Audio Device Manager
    juce::AudioDeviceManager deviceManager;
    juce::String err = deviceManager.initialiseWithDefaultDevices(2, 2);
    
    if (err.isNotEmpty()) {
        std::cout << "Error initializing audio device: " << err.toStdString() << "\n";
    }

    if (auto* device = deviceManager.getCurrentAudioDevice()) {
        std::cout << "JUCE Default Audio Device: " << device->getName().toStdString() << "\n";
    }

    // Hook our audio engine into the soundcard
    deviceManager.addAudioCallback(&engine);

    // Create the Main Window
    gui::MainWindow window(&engine);
    window.showMaximized();

    int result = app.exec();

    // Clean up
    deviceManager.removeAudioCallback(&engine);

    return result;
}
