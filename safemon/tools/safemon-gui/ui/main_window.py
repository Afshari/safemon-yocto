"""
main_window.py - Main application window.
Contains the tab bar and hosts all tab widgets.
"""

from PyQt6.QtWidgets import (
    QMainWindow, QWidget, QTabWidget, QVBoxLayout, QLabel, QPushButton
)
from PyQt6.QtCore import Qt

from ui.key_management import KeyManagementTab
from ui.sign_verify import SignVerifyTab
from ui.device_status import DeviceStatusTab

from core.device_session import DeviceSession
from ui.device_topbar import DeviceTopBar
from ui.device_files import DeviceFilesTab

class MainWindow(QMainWindow):

    WINDOW_WIDTH  = 1000
    WINDOW_HEIGHT = 700
    LEFT_PANEL_WIDTH = 300

    def __init__(self):
        super().__init__()
        self.setWindowTitle("Safemon")
        self.setFixedSize(self.WINDOW_WIDTH, self.WINDOW_HEIGHT)
        self.session = DeviceSession()
        self._build_ui()

    def _build_ui(self):
        central = QWidget()
        self.setCentralWidget(central)

        layout = QVBoxLayout(central)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)

        self.topbar = DeviceTopBar(self.session)
        layout.addWidget(self.topbar)

        self.tabs = QTabWidget()
        self.tabs.setDocumentMode(True)

        # --- Placeholder tabs - each will be replaced with real widget ---
        self.tabs.addTab(KeyManagementTab(self.session),   "Key Management")
        self.tabs.addTab(SignVerifyTab(),                  "Sign / Verify")
        self.tabs.addTab(_placeholder("Fault Monitor"),   "Fault Monitor")
        self.tabs.addTab(DeviceFilesTab(self.session),      "Device Files")
        self.tabs.addTab(DeviceStatusTab(self.session),     "Device Status")

        layout.addWidget(self.tabs)


def _placeholder(name: str) -> QWidget:
    """Temporary placeholder widget for a tab not yet implemented."""
    w = QWidget()
    layout = QVBoxLayout(w)
    layout.setAlignment(Qt.AlignmentFlag.AlignCenter)

    lbl = QLabel(f"{name} - coming soon")
    lbl.setAlignment(Qt.AlignmentFlag.AlignCenter)
    lbl.setStyleSheet("color: #555555; font-size: 16px;")

    btn = QPushButton("Test Button")
    btn.setFixedWidth(160)

    layout.addWidget(lbl)
    layout.addSpacing(12)
    layout.addWidget(btn, alignment=Qt.AlignmentFlag.AlignCenter)
    return w