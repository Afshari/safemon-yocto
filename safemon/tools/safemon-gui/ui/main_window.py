"""
main_window.py - Main application window.
Contains the tab bar and hosts all tab widgets.
"""

from PyQt6.QtWidgets import (
    QMainWindow, QWidget, QTabWidget, QVBoxLayout, QLabel
)
from PyQt6.QtCore import Qt


class MainWindow(QMainWindow):

    WINDOW_WIDTH  = 1000
    WINDOW_HEIGHT = 700
    LEFT_PANEL_WIDTH = 300

    def __init__(self):
        super().__init__()
        self.setWindowTitle("Safemon")
        self.setFixedSize(self.WINDOW_WIDTH, self.WINDOW_HEIGHT)
        self._build_ui()

    def _build_ui(self):
        central = QWidget()
        self.setCentralWidget(central)

        layout = QVBoxLayout(central)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        self.tabs = QTabWidget()
        self.tabs.setDocumentMode(True)

        # --- Placeholder tabs - each will be replaced with real widget ---
        self.tabs.addTab(_placeholder("Key Management"),  "Key Management")
        self.tabs.addTab(_placeholder("Sign / Verify"),   "Sign / Verify")
        self.tabs.addTab(_placeholder("Fault Monitor"),   "Fault Monitor")
        self.tabs.addTab(_placeholder("Device Files"),    "Device Files")
        self.tabs.addTab(_placeholder("Device Status"),   "Device Status")

        layout.addWidget(self.tabs)


def _placeholder(name: str) -> QWidget:
    """Temporary placeholder widget for a tab not yet implemented."""
    w = QWidget()
    lbl = QLabel(f"{name} - coming soon")
    lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
    lbl.setStyleSheet("color: #555555; font-size: 16px;")
    layout = QVBoxLayout(w)
    layout.addWidget(lbl)
    return w