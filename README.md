# Graphite Guitar DAW
[![Ask DeepWiki](https://devin.ai/assets/askdeepwiki.png)](https://deepwiki.com/cataclysmnik/guitar-daw)

Graphite is a lightweight, performant Digital Audio Workstation (DAW) designed for guitarists. Built with Python and PySide6, it provides a clean, native-feeling interface for recording, processing, and arranging guitar tracks. It leverages the `pedalboard` library for powerful real-time audio effects, including support for VST3 plugins. The UI is heavily customized with a sleek, dark aesthetic inspired by the "Nothing" design language.

## Key Features

- **Real-time Audio Engine:** Low-latency audio processing powered by `sounddevice` and `pedalboard`. Supports ASIO, WASAPI, and MME for wide hardware compatibility.
- **VST3 Plugin Support:** Load, manage, and save your favorite VST3 plugins within the effects rack. The engine isolates plugin instances to prevent state conflicts.
- **Multi-Track Timeline:** A non-destructive timeline editor for arranging, moving, and resizing audio clips.
- **Integrated Effects & Utilities:**
    - A suite of built-in effects including a custom `TubeOverdrive`, Reverb, Delay, Chorus, and Compressor.
    - A high-precision chromatic **Guitar Tuner**.
    - A visual **Metronome** with tap tempo.
- **Advanced Mixer:** A dedicated mixer view with faders, pan knobs, and level meters for each track.
- **Offline Rendering:** Export your projects to high-quality WAV (16/24-bit) or MP3 files.
- **Project Management:** Save and load your complete sessions, including track settings, audio clips, and full VST3 plugin states, in the `.graphite` file format.
- **Custom UI:** A fast, responsive, and visually distinct user interface built from the ground up with PySide6, featuring a custom frameless window implementation for a modern look on Windows.

## Technology Stack

- **Core:** Python 3
- **GUI:** PySide6 (Qt for Python)
- **Audio I/O:** `sounddevice`
- **Effects & VST Hosting:** `pedalboard`
- **DSP & Data Manipulation:** `numpy`
- **Audio File I/O:** `soundfile`

## Getting Started

### Prerequisites

- Python 3.8+
- A VST3-compatible audio plugin host (if you intend to use VST3 plugins)
- An audio interface (ASIO drivers recommended on Windows for best performance)

### Installation

1.  **Clone the repository:**
    ```sh
    git clone https://github.com/cataclysmnik/guitar-daw.git
    cd guitar-daw
    ```

2.  **Install the required Python packages:**
    ```sh
    pip install -r requirements.txt
    ```
    The key dependencies are:
    - `PySide6`
    - `numpy`
    - `sounddevice`
    - `pedalboard`
    - `soundfile`

### Running the Application

Execute the main script to launch the DAW:

```sh
python main.py
```

## Usage

1.  **Audio Setup:**
    - On first launch, navigate to `Audio > Audio Device Settings...` (or press `Ctrl+,`).
    - Select your audio system (e.g., ASIO, WASAPI) and your audio interface as the input/output device.
    - Configure your channel ranges and buffer size for optimal latency.

2.  **Adding Effects:**
    - Select a track by clicking on its header in the left panel.
    - The bottom dock will show the "Effects" tab for the selected track.
    - Use the "Add Built-in Effect" dropdown or the "Load VST3" button to build your effects chain.
    - Drag and drop effects in the rack or the right-hand signal flow view to reorder them.

3.  **Recording:**
    - Arm a track by clicking the 'R' button on its card.
    - Click the record button (●) in the timeline toolbar or press `R` to start recording.
    - Click play/pause or stop to end the recording. The new audio clip will appear on the timeline.

4.  **Arrangement:**
    - Click and drag audio clips on the timeline to move them.
    - Hover over the left or right edge of a clip and drag to resize it.
    - Drag and drop WAV files from your file explorer directly onto the timeline to import them.

5.  **Saving and Loading:**
    - Use `File > Save Project` (`Ctrl+S`) to save your session as a `.graphite` file.
    - Use `File > Open Project...` (`Ctrl+O`) to load a previously saved session.

## Project Structure

- `main.py`: The application entry point. Initializes the QApplication and MainWindow.
- `audio_engine.py`: The core of the DAW. Manages audio I/O streams, track-based DSP, mixing, recording logic, and VST handling.
- `project_manager.py`: Handles serialization and deserialization of the entire project state to/from `.graphite` JSON files.
- `theme_utils.py`: Contains helper functions and mixins for the custom frameless window and dark theme styling on Windows.
- `widgets/`: This directory contains all the PySide6 UI components.
  - `main_window.py`: The main application window, orchestrating all other widgets.
  - `track_card.py`: The UI for a single track header in the left-side panel.
  - `effects_rack.py`: The UI for managing a track's effects chain, including VST loading menus.
  - `timeline.py`: Contains the time ruler, transport controls, and the scrollable lanes for audio clips.
  - `mixer.py`: The dedicated channel strip and master fader view.
  - `tuner.py` & `metronome.py`: The integrated guitar tuner and metronome widgets.
  - `audio_settings.py`: The advanced audio device configuration dialog.
  - `signal_flow.py`: The vertical node-based view of the effects chain.
  - `splash.py`: The custom-drawn startup splash screen.