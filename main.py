import os
os.environ["SD_ENABLE_ASIO"] = "1"

# Import and initialize pedalboard early to force C++/JUCE/COM initialization
# before PySide6/Qt creates its QApplication/thread structures.
import numpy as np
import pedalboard
try:
    _dummy_board = pedalboard.Pedalboard([])
    _dummy_board(np.zeros((1, 128), dtype=np.float32), 44100)
except Exception:
    pass

import sys
from PySide6.QtWidgets import QApplication
from PySide6.QtCore import Qt
from widgets.main_window import MainWindow
from widgets.splash import GraphiteSplashScreen

def main():
    # Setup application properties
    QApplication.setApplicationName("Graphite DAW")
    QApplication.setApplicationDisplayName("Graphite DAW")
    QApplication.setOrganizationName("Graphite Studio")
    
    # Initialize PySide Application
    app = QApplication(sys.argv)
    
    # Create and show startup splash screen
    splash = GraphiteSplashScreen()
    splash.show()
    app.processEvents()
    
    # Create the DAW Main Window interface
    window = MainWindow(splash=splash)
    window.showMaximized()
    window.raise_()
    window.activateWindow()
    
    # Close the splash screen after the main window is fully loaded
    splash.finish(window)
    
    # Run event loop
    exit_code = app.exec()
    
    # Clean up temp VSTs on exit
    try:
        from audio_engine import clean_temp_vsts
        clean_temp_vsts()
    except Exception:
        pass
        
    sys.exit(exit_code)

if __name__ == "__main__":
    main()
