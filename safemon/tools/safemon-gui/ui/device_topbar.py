"""
device_topbar.py - Persistent top bar for selecting target device and credentials.
Shared across all tabs via a single DeviceSession instance.
"""

from PyQt6.QtWidgets import (
    QWidget, QHBoxLayout, QLabel, QLineEdit, QComboBox
)


PLATFORM_DISPLAY_NAMES = {
    "raspberry_pi": "Raspberry Pi",
    "jetson_orin_nano": "Jetson Orin Nano",
    "qemu": "QEMU",
}
PLATFORM_KEYS_BY_NAME = {v: k for k, v in PLATFORM_DISPLAY_NAMES.items()}


class DeviceTopBar(QWidget):

    def __init__(self, session, parent=None):
        super().__init__(parent)
        self._session = session
        self.setObjectName("topBar")
        self.setStyleSheet("#topBar { background-color: #1c1c1c; }")
        self.setFixedHeight(48)
        self._build_ui()

    def _build_ui(self):
        layout = QHBoxLayout(self)
        layout.setContentsMargins(16, 6, 16, 6)
        layout.setSpacing(12)

        layout.addWidget(QLabel("Target:"))

        self._target_combo = QComboBox()
        self._target_combo.addItems(list(PLATFORM_DISPLAY_NAMES.values()))
        self._target_combo.currentTextChanged.connect(self._on_target_changed)
        layout.addWidget(self._target_combo)

        layout.addWidget(QLabel("Username:"))
        self._user_field = QLineEdit(self._session.username)
        self._user_field.setFixedWidth(100)
        self._user_field.textChanged.connect(self._on_username_changed)
        layout.addWidget(self._user_field)

        layout.addWidget(QLabel("Password:"))
        self._pass_field = QLineEdit()
        self._pass_field.setEchoMode(QLineEdit.EchoMode.Password)
        self._pass_field.setPlaceholderText("leave empty if none")
        self._pass_field.setFixedWidth(140)
        self._pass_field.textChanged.connect(self._on_password_changed)
        layout.addWidget(self._pass_field)

        layout.addStretch()

    def _on_target_changed(self, display_name):
        self._session.platform_key = PLATFORM_KEYS_BY_NAME[display_name]

    def _on_username_changed(self, text):
        self._session.username = text

    def _on_password_changed(self, text):
        self._session.password = text