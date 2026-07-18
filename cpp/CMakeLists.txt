cmake_minimum_required(VERSION 3.20)
project(GuitarDAW VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# -----------------------------------------------------------------------------
# JUCE Setup
# -----------------------------------------------------------------------------
# Using JUCE for robust audio backend, VST hosting, and MIDI
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.0
)
FetchContent_MakeAvailable(juce)

# -----------------------------------------------------------------------------
# Neural Amp Modeler Core Setup
# -----------------------------------------------------------------------------
FetchContent_Declare(
    nam
    GIT_REPOSITORY https://github.com/sdatkinson/NeuralAmpModelerCore.git
    GIT_TAG        main
)
# NeuralAmpModelerCore doesn't have a perfectly clean CMake for simple subdirectories without its own deps,
# but we can try bringing it in or building its source files directly if needed.
FetchContent_MakeAvailable(nam)

# -----------------------------------------------------------------------------
# Steinberg ASIO SDK Setup (Required for JUCE ASIO support on Windows)
# -----------------------------------------------------------------------------
FetchContent_Declare(
    asiosdk
    URL https://www.steinberg.net/asiosdk
)
FetchContent_MakeAvailable(asiosdk)

# -----------------------------------------------------------------------------
# Executable Setup
# -----------------------------------------------------------------------------
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_PREFIX_PATH "D:/Qt/6.11.1/msvc2022_64" CACHE PATH "Path to Qt" FORCE)

find_package(Qt6 COMPONENTS Core Gui Widgets Svg REQUIRED)

# -----------------------------------------------------------------------------
# Target Configuration
# -----------------------------------------------------------------------------
add_executable(GuitarDAW_CPP
    src/core/main.cpp
    src/audio/AudioEngine.cpp
    src/audio/NamEffect.cpp
    src/audio/NamAudioProcessor.cpp
    src/gui/AudioSettingsDialog.cpp
    src/gui/StyleTheme.cpp
    src/gui/CustomTitleBar.cpp
    src/gui/MainWindow.cpp
    src/gui/TrackCard.cpp
    src/gui/Timeline.cpp
    src/gui/MixerStrip.cpp
    src/gui/MixerPanel.cpp
    src/gui/EffectsRack.cpp
    src/gui/SignalFlow.cpp
    src/gui/CustomKnob.cpp
    src/gui/LevelMeter.cpp
    resources.qrc
)

target_include_directories(GuitarDAW_CPP PRIVATE
    src
    ${nam_SOURCE_DIR}
    ${nam_SOURCE_DIR}/Dependencies/eigen
    ${nam_SOURCE_DIR}/Dependencies/nlohmann
)

# Because NeuralAmpModelerCore doesn't define a CMake target, we include its sources directly
file(GLOB_RECURSE NAM_SOURCES "${nam_SOURCE_DIR}/NAM/*.cpp")
target_sources(GuitarDAW_CPP PRIVATE ${NAM_SOURCES})

# Force NAM Core to use float processing instead of double to match JUCE and save CPU
target_compile_definitions(GuitarDAW_CPP PRIVATE NAM_SAMPLE_FLOAT)

# Disable JUCE assertions to prevent debug breaks caused by buggy third-party plugins (e.g. Neural DSP background thread callbacks)
target_compile_definitions(GuitarDAW_CPP PRIVATE JUCE_DISABLE_ASSERTS=1)

# Enable ASIO globally for the juce_audio_devices module
target_compile_definitions(juce_audio_devices INTERFACE JUCE_ASIO=1)
target_include_directories(juce_audio_devices INTERFACE
    ${asiosdk_SOURCE_DIR}/common
    ${asiosdk_SOURCE_DIR}/host
    ${asiosdk_SOURCE_DIR}/host/pc
)

# We link JUCE recommended targets (juce_audio_devices, juce_core, etc.)
# We link Qt6::Widgets and Qt6::Svg
target_link_libraries(GuitarDAW_CPP PRIVATE
    Qt6::Widgets
    Qt6::Svg
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
)

target_compile_definitions(GuitarDAW_CPP PRIVATE JUCE_PLUGINHOST_VST3=1 JUCE_PLUGINHOST_VST=0 JUCE_PLUGINHOST_AU=0)

# -----------------------------------------------------------------------------
# Deployment Setup (Windows)
# -----------------------------------------------------------------------------
# Automatically copy Qt DLLs (like Qt6Widgetsd.dll) to the build folder so it runs!
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "D:/Qt/6.11.1/msvc2022_64/bin")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET GuitarDAW_CPP POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" 
            --dir "$<TARGET_FILE_DIR:GuitarDAW_CPP>"
            "$<TARGET_FILE:GuitarDAW_CPP>"
            COMMENT "Running windeployqt to copy required Qt DLLs..."
        )
    endif()
endif()
