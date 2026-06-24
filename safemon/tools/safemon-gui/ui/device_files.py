"""
device_files.py - Device Files tab.
Two sections:
  1. Known Files - predefined config/sig files with fixed device paths, read-only preview
  2. File Transfer - copy any local file to a custom destination path on the device
"""

import os
import tempfile
from pathlib import Path

from PyQt6.QtWidgets import (
    QWidget, QHBoxLayout, QVBoxLayout, QLabel, QPushButton,
    QLineEdit, QFileDialog, QGroupBox, QComboBox, QTextEdit,
    QMessageBox, QFrame
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
        self._local_known_path = None
        self._local_transfer_path = None
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

        layout.addWidget(self._build_known_files_group())
        layout.addWidget(self._build_divider())
        layout.addWidget(self._build_transfer_group())
        layout.addStretch()

        return panel

    def _build_known_files_group(self):
        group = QGroupBox("Known Files")
        layout = QVBoxLayout(group)
        layout.setSpacing(6)

        self._file_combo = QComboBox()
        self._file_combo.addItems(list(self._files.keys()))
        self._file_combo.currentTextChanged.connect(self._on_known_file_selected)

        self._device_path_label = QLabel("Device: -")
        self._device_path_label.setStyleSheet("color: grey; font-size: 11px;")
        self._device_path_label.setWordWrap(True)

        self._local_known_label = QLabel("Local: -")
        self._local_known_label.setStyleSheet("color: grey; font-size: 11px;")
        self._local_known_label.setWordWrap(True)

        browse_known_btn = QPushButton("Browse Local File")
        browse_known_btn.clicked.connect(self._browse_known_file)

        load_btn = QPushButton("Load from Device")
        load_btn.clicked.connect(self._load_from_device)

        copy_known_btn = QPushButton("Copy to Device")
        copy_known_btn.clicked.connect(self._copy_known_to_device)

        layout.addWidget(QLabel("File:"))
        layout.addWidget(self._file_combo)
        layout.addWidget(self._device_path_label)
        layout.addWidget(self._local_known_label)
        layout.addWidget(browse_known_btn)
        layout.addWidget(load_btn)
        layout.addWidget(copy_known_btn)

        self._on_known_file_selected(self._file_combo.currentText())
        return group

    def _build_divider(self):
        line = QFrame()
        line.setFrameShape(QFrame.Shape.HLine)
        line.setStyleSheet("color: #666;")
        return line

    def _build_transfer_group(self):
        group = QGroupBox("File Transfer")
        layout = QVBoxLayout(group)
        layout.setSpacing(6)

        self._transfer_local_label = QLabel("Local: -")
        self._transfer_local_label.setStyleSheet("color: grey; font-size: 11px;")
        self._transfer_local_label.setWordWrap(True)

        browse_transfer_btn = QPushButton("Browse Local File")
        browse_transfer_btn.clicked.connect(self._browse_transfer_file)

        self._dest_field = QLineEdit()
        self._dest_field.setPlaceholderText("Destination path on device (e.g. /home/root/)")

        copy_transfer_btn = QPushButton("Copy to Device")
        copy_transfer_btn.clicked.connect(self._copy_transfer_to_device)

        layout.addWidget(self._transfer_local_label)
        layout.addWidget(browse_transfer_btn)
        layout.addWidget(QLabel("Destination on device:"))
        layout.addWidget(self._dest_field)
        layout.addWidget(copy_transfer_btn)

        return group

    # ------------------------------------------------------------------
    # Right panel
    # ------------------------------------------------------------------

    def _build_right_panel(self):
        panel = QWidget()
        layout = QVBoxLayout(panel)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(8)

        header = QLabel("File Preview")
        header.setStyleSheet("font-size: 14px; color: grey;")

        self._content_box = QTextEdit()
        self._content_box.setReadOnly(True)
        self._content_box.setPlaceholderText(
            "Select a known file and click 'Browse Local File' or "
            "'Load from Device' to preview its content here.\n\n"
            "Files in the File Transfer section do not show a preview."
        )

        self._log_box = QTextEdit()
        self._log_box.setReadOnly(True)
        self._log_box.setFixedHeight(100)
        self._log_box.setPlaceholderText("Status messages will appear here...")

        layout.addWidget(header)
        layout.addWidget(self._content_box, stretch=1)
        layout.addWidget(self._log_box, stretch=0)

        return panel

    # ------------------------------------------------------------------
    # Known Files - handlers
    # ------------------------------------------------------------------

    def _on_known_file_selected(self, friendly_name):
        device_path = self._files.get(friendly_name, "")
        self._device_path_label.setText(f"Device: {device_path}")
        self._local_known_label.setText("Local: -")
        self._local_known_path = None
        if hasattr(self, "_content_box"):
            self._content_box.clear()

    def _browse_known_file(self):
        path, _ = QFileDialog.getOpenFileName(self, "Select Local File")
        if not path:
            return
        self._local_known_path = path
        self._local_known_label.setText(f"Local: {path}")
        try:
            with open(path, "r", encoding="utf-8", errors="replace") as f:
                content = f.read()
            self._content_box.setPlainText(content)
        except Exception as e:
            self._log(f"ERROR: Could not read file for preview: {e}")

    def _load_from_device(self):
        friendly_name = self._file_combo.currentText()
        device_path = self._files.get(friendly_name, "")
        if not device_path:
            self._log("ERROR: No device path for selected file.")
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
        self._log("OK: File loaded from device (read-only preview).")

    def _on_load_failed(self, error_text):
        self._log(f"ERROR: {error_text}")

    def _copy_known_to_device(self):
        if not self._local_known_path:
            self._log("ERROR: No local file selected. Use Browse to select a file first.")
            return

        friendly_name = self._file_combo.currentText()
        device_path = self._files.get(friendly_name, "")
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

        self._log(f"Copying {self._local_known_path} -> {platform_key}:{device_path} ...")

        self._copy_worker = Worker(
            self._do_copy, self._local_known_path, device_path, host, port, username, password
        )
        self._copy_worker.finished_ok.connect(self._on_copy_success)
        self._copy_worker.failed.connect(self._on_copy_failed)
        self._copy_worker.start()

    # ------------------------------------------------------------------
    # File Transfer - handlers
    # ------------------------------------------------------------------

    def _browse_transfer_file(self):
        path, _ = QFileDialog.getOpenFileName(self, "Select Local File")
        if not path:
            return
        self._local_transfer_path = path
        self._transfer_local_label.setText(f"Local: {path}")
        self._content_box.setPlaceholderText(
            f"'{Path(path).name}' selected for transfer.\n\n"
            "No preview available for File Transfer section."
        )
        self._content_box.clear()

    def _copy_transfer_to_device(self):
        if not self._local_transfer_path:
            self._log("ERROR: No local file selected. Use Browse to select a file first.")
            return

        dest = self._dest_field.text().strip()
        if not dest:
            self._log("ERROR: No destination path entered.")
            return

        host, port, username, password, platform_key = self._connection_info()
        if not host:
            self._log(f"ERROR: No host configured for {platform_key}.")
            return

        filename = Path(self._local_transfer_path).name
        if dest.endswith("/"):
            dest = dest + filename
        else:
            # if dest looks like a directory (no extension), append filename automatically
            remote_path = Path(dest)
            if "." not in remote_path.name:
                dest = dest + "/" + filename

        confirm = QMessageBox.question(
            self,
            "Confirm Copy to Device",
            f"Copy:\n{self._local_transfer_path}\n\nTo {platform_key}:\n{dest}\n\nAre you sure?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if confirm != QMessageBox.StandardButton.Yes:
            self._log("Copy cancelled.")
            return

        self._log(f"Copying {self._local_transfer_path} -> {platform_key}:{dest} ...")

        self._transfer_worker = Worker(
            self._do_copy, self._local_transfer_path, dest, host, port, username, password
        )
        self._transfer_worker.finished_ok.connect(self._on_copy_success)
        self._transfer_worker.failed.connect(self._on_copy_failed)
        self._transfer_worker.start()

    # ------------------------------------------------------------------
    # Shared copy task
    # ------------------------------------------------------------------

    def _do_copy(self, local_path, device_path, host, port, username, password):
        mgr = SSHManager(host, port, username, password)
        mgr.connect()
        try:
            import posixpath
            remote_dir = posixpath.dirname(device_path)
            mgr.run_command(f"mkdir -p {remote_dir}")
            mgr.put_file(local_path, device_path)
        finally:
            mgr.close()
        return device_path

    def _on_copy_success(self, device_path):
        self._log(f"OK: File copied to device -> {device_path}")

    def _on_copy_failed(self, error_text):
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