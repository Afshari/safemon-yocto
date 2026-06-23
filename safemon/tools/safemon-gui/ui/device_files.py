"""
device_files.py - Device Files tab.
Pull, edit, and push important text files (config, signature, boot, etc.)
to/from the device using the shared SSH session.
"""

from pathlib import Path

from PyQt6.QtWidgets import (
    QWidget, QHBoxLayout, QVBoxLayout, QLabel, QPushButton,
    QLineEdit, QFileDialog, QGroupBox, QComboBox, QTextEdit,
    QMessageBox
)
from PyQt6.QtCore import Qt

from core.workers import Worker
from core.ssh_manager import SSHManager
from core.config_manager import load_platform, load_device_files


class DeviceFilesTab(QWidget):

    def __init__(self, session, parent=None):
        super().__init__(parent)
        self._session = session
        self._files = load_device_files()
        self._build_ui()

    def _build_ui(self):
        root = QHBoxLayout(self)
        root.setContentsMargins(0, 0, 0, 0)
        root.setSpacing(0)

        root.addWidget(self._build_left_panel(), stretch=0)
        root.addWidget(self._build_right_panel(), stretch=1)

    # ------------------------------------------------------------------
    # Left panel
    # ------------------------------------------------------------------

    def _build_left_panel(self):
        panel = QWidget()
        panel.setFixedWidth(300)
        panel.setObjectName("leftPanel")
        panel.setStyleSheet("#leftPanel { background-color: #4a4a4a; }")

        layout = QVBoxLayout(panel)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(12)

        file_group = QGroupBox("File")
        file_layout = QVBoxLayout(file_group)
        file_layout.setSpacing(6)

        self._file_combo = QComboBox()
        self._file_combo.addItems(list(self._files.keys()))
        self._file_combo.currentTextChanged.connect(self._on_file_selected)

        self._path_field = QLineEdit()
        self._path_field.setReadOnly(True)

        file_layout.addWidget(QLabel("Known file:"))
        file_layout.addWidget(self._file_combo)
        file_layout.addWidget(QLabel("Device path:"))
        file_layout.addWidget(self._path_field)

        browse_btn = QPushButton("Browse Local File")
        browse_btn.clicked.connect(self._browse_local_file)

        load_btn = QPushButton("Load from Device")
        load_btn.clicked.connect(self._load_from_device)

        push_btn = QPushButton("Copy to Device")
        push_btn.clicked.connect(self._copy_to_device)

        layout.addWidget(file_group)
        layout.addWidget(browse_btn)
        layout.addWidget(load_btn)
        layout.addWidget(push_btn)
        layout.addStretch()

        self._on_file_selected(self._file_combo.currentText())

        return panel

    # ------------------------------------------------------------------
    # Right panel
    # ------------------------------------------------------------------

    def _build_right_panel(self):
        panel = QWidget()
        layout = QVBoxLayout(panel)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(8)

        header = QLabel("File Content")
        header.setStyleSheet("font-size: 14px; color: grey;")

        self._content_box = QTextEdit()
        self._content_box.setPlaceholderText("Load a file from the device, or browse a local file...")

        self._log_box = QTextEdit()
        self._log_box.setReadOnly(True)
        self._log_box.setFixedHeight(100)
        self._log_box.setPlaceholderText("Status messages will appear here...")

        layout.addWidget(header)
        layout.addWidget(self._content_box, stretch=1)
        layout.addWidget(self._log_box, stretch=0)

        return panel

    # ------------------------------------------------------------------
    # File selection
    # ------------------------------------------------------------------

    def _on_file_selected(self, friendly_name):
        device_path = self._files.get(friendly_name, "")
        self._path_field.setText(device_path)

    # ------------------------------------------------------------------
    # Browse local
    # ------------------------------------------------------------------

    def _browse_local_file(self):
        path, _ = QFileDialog.getOpenFileName(self, "Select Local File")
        if not path:
            return
        try:
            with open(path, "r", encoding="utf-8", errors="replace") as f:
                content = f.read()
        except Exception as e:
            self._log(f"ERROR: Could not read local file: {e}")
            return
        self._content_box.setPlainText(content)
        self._log(f"Loaded local file into editor: {path}")

    # ------------------------------------------------------------------
    # Load from device
    # ------------------------------------------------------------------

    def _load_from_device(self):
        device_path = self._path_field.text().strip()
        if not device_path:
            self._log("ERROR: No device path selected.")
            return

        host, port, username, password, platform_key = self._connection_info()
        if not host:
            self._log(f"ERROR: No host configured for {platform_key}.")
            return

        self._log(f"Loading {device_path} from {platform_key}...")

        self._load_worker = Worker(
            self._do_load, device_path, host, port, username, password
        )
        self._load_worker.finished_ok.connect(self._on_load_success)
        self._load_worker.failed.connect(self._on_load_failed)
        self._load_worker.start()

    def _do_load(self, device_path, host, port, username, password):
        import tempfile, os

        mgr = SSHManager(host, port, username, password)
        mgr.connect()
        try:
            fd, tmp_path = tempfile.mkstemp()
            os.close(fd)
            mgr.get_file(device_path, tmp_path)
            with open(tmp_path, "r", encoding="utf-8", errors="replace") as f:
                content = f.read()
            os.remove(tmp_path)
        finally:
            mgr.close()
        return content

    def _on_load_success(self, content):
        self._content_box.setPlainText(content)
        self._log("OK: File loaded from device.")

    def _on_load_failed(self, error_text):
        self._log(f"ERROR: {error_text}")

    # ------------------------------------------------------------------
    # Copy to device
    # ------------------------------------------------------------------

    def _copy_to_device(self):
        device_path = self._path_field.text().strip()
        if not device_path:
            self._log("ERROR: No device path selected.")
            return

        content = self._content_box.toPlainText()
        if not content:
            self._log("ERROR: Nothing to copy - the content box is empty.")
            return

        host, port, username, password, platform_key = self._connection_info()
        if not host:
            self._log(f"ERROR: No host configured for {platform_key}.")
            return

        confirm = QMessageBox.question(
            self,
            "Confirm Copy to Device",
            f"This will overwrite:\n\n{device_path}\n\non {platform_key}.\n\nAre you sure?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if confirm != QMessageBox.StandardButton.Yes:
            self._log("Copy cancelled.")
            return

        self._log(f"Copying to {device_path} on {platform_key}...")

        self._push_worker = Worker(
            self._do_push, content, device_path, host, port, username, password
        )
        self._push_worker.finished_ok.connect(self._on_push_success)
        self._push_worker.failed.connect(self._on_push_failed)
        self._push_worker.start()

    def _do_push(self, content, device_path, host, port, username, password):
        import tempfile, os

        fd, tmp_path = tempfile.mkstemp()
        with os.fdopen(fd, "w", encoding="utf-8") as f:
            f.write(content)

        mgr = SSHManager(host, port, username, password)
        mgr.connect()
        try:
            mgr.put_file(tmp_path, device_path)
        finally:
            mgr.close()
            os.remove(tmp_path)

        return device_path

    def _on_push_success(self, device_path):
        self._log(f"OK: File copied to device -> {device_path}")

    def _on_push_failed(self, error_text):
        self._log(f"ERROR: {error_text}")

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _connection_info(self):
        platform_key = self._session.platform_key
        config = load_platform(platform_key)
        host = config.get("host", "")
        port = config.get("port", 22)
        username = self._session.username or "root"
        password = self._session.password
        return host, port, username, password, platform_key

    def _log(self, message: str):
        self._log_box.append(message)