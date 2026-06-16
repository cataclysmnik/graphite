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

def main():
    # Setup application properties
    QApplication.setApplicationName("Graphite DAW")
    QApplication.setApplicationDisplayName("Graphite DAW")
    QApplication.setOrganizationName("Graphite Studio")
    
    # Initialize PySide Application
    app = QApplication(sys.argv)
    
    # Create the DAW Main Window interface
    window = MainWindow()
    window.show()
    window.raise_()
    window.activateWindow()
    
    # Run event loop
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
