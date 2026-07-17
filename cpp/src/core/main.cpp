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
    
    // Determine Settings File Location: %APPDATA%/GuitarDaw/AudioSettings.xml
    juce::File appDataDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("GuitarDaw");
    if (!appDataDir.exists()) {
        appDataDir.createDirectory();
    }
    juce::File audioSettingsFile = appDataDir.getChildFile("AudioSettings.xml");
    
    // Attempt to load settings
    std::unique_ptr<juce::XmlElement> savedAudioState = nullptr;
    if (audioSettingsFile.existsAsFile()) {
        savedAudioState = juce::XmlDocument::parse(audioSettingsFile);
    }
    
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

    // Create the Main Window
    gui::MainWindow window(&engine, &deviceManager);
    window.showMaximized();

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
