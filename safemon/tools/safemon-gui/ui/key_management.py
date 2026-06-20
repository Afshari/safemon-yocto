"""
key_management.py - Key Management tab.
Handles key pair generation and copying public key to device via SCP.
"""

import os
import sys
from pathlib import Path

SAFEMON_SIGN_DIR = Path(__file__).resolve().parent.parent.parent / "safemon-sign"
print(SAFEMON_SIGN_DIR)
sys.path.insert(0, str(SAFEMON_SIGN_DIR))

from PyQt6.QtWidgets import (
    QWidget, QHBoxLayout, QVBoxLayout, QLabel, QPushButton,
    QLineEdit, QFileDialog, QGroupBox, QComboBox, QTextEdit,
    QSizePolicy
)
from PyQt6.QtCore import Qt
from core.workers import Worker

from core.ssh_manager import SSHManager
from core.config_manager import load_platform

class KeyManagementTab(QWidget):

    def __init__(self, session, parent=None):
        super().__init__(parent)
        self._session = session
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

        # --- Key output directory ---
        dir_group = QGroupBox("Output Directory")
        dir_layout = QVBoxLayout(dir_group)
        dir_layout.setSpacing(6)

        self._key_dir_field = QLineEdit()
        self._key_dir_field.setText(str(Path.home() / ".safemon"))
        self._key_dir_field.setPlaceholderText("Key output directory")

        browse_btn = QPushButton("Browse")
        browse_btn.clicked.connect(self._browse_key_dir)

        dir_layout.addWidget(self._key_dir_field)
        dir_layout.addWidget(browse_btn)

        # --- Generate button ---
        generate_btn = QPushButton("Generate Key Pair")
        generate_btn.clicked.connect(self._generate_keys)

        # --- SCP section ---
        scp_group = QGroupBox("Copy to Device")
        scp_layout = QVBoxLayout(scp_group)
        scp_layout.setSpacing(6)

        self._scp_btn = QPushButton("Copy Public Key to Device")
        self._scp_btn.clicked.connect(self._copy_pubkey_to_device)

        scp_layout.addWidget(self._scp_btn)

        # --- Assemble ---
        layout.addWidget(dir_group)
        layout.addWidget(generate_btn)
        layout.addSpacing(8)
        layout.addWidget(scp_group)
        layout.addStretch()

        return panel

    # ------------------------------------------------------------------
    # Right panel
    # ------------------------------------------------------------------

    def _build_right_panel(self):
        panel = QWidget()
        layout = QVBoxLayout(panel)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(8)

        header = QLabel("Key Generation Output")
        header.setStyleSheet("font-size: 14px; color: #aaaaaa;")

        self._output = QTextEdit()
        self._output.setReadOnly(True)
        self._output.setPlaceholderText("Key generation results will appear here...")

        layout.addWidget(header)
        layout.addWidget(self._output)

        return panel

    # ------------------------------------------------------------------
    # Slots
    # ------------------------------------------------------------------

    def _browse_key_dir(self):
        current = self._key_dir_field.text() or str(Path.home())
        chosen = QFileDialog.getExistingDirectory(
            self, "Select Key Output Directory", current
        )
        if chosen:
            self._key_dir_field.setText(chosen)

    def _generate_keys(self):
        key_dir = Path(self._key_dir_field.text().strip())

        if not key_dir:
            self._log("ERROR: No output directory specified.")
            return

        try:
            key_dir.mkdir(parents=True, exist_ok=True)
        except Exception as e:
            self._log(f"ERROR: Could not create directory: {e}")
            return

        key_path = key_dir / "safemon.key"

        self._worker = Worker(self._do_generate_keys, key_path)
        self._worker.finished_ok.connect(self._on_generate_success)
        self._worker.failed.connect(self._on_generate_failed)
        self._worker.start()

    def _do_generate_keys(self, key_path):
        from safemon_sign import random_scalar, G, save_keypair, save_pubkey

        private_key = random_scalar()
        public_key = private_key * G

        pub_path = key_path.with_suffix(".pub")

        save_keypair(str(key_path), private_key, public_key)
        save_pubkey(str(pub_path), public_key)

        return key_path, pub_path

    def _on_generate_success(self, result):
        key_path, pub_path = result
        self._log(f"OK: Private key -> {key_path}")
        self._log(f"OK: Public key  -> {pub_path}")

    def _on_generate_failed(self, error_text):
        self._log(f"ERROR: {error_text}")

    def _log(self, message: str):
        self._output.append(message)
        
        
    def _copy_pubkey_to_device(self):
        key_dir = Path(self._key_dir_field.text().strip())
        pub_path = key_dir / "safemon.pub"

        if not pub_path.exists():
            self._log(f"ERROR: Public key not found at {pub_path}. Generate a key pair first.")
            return

        platform_key = self._session.platform_key
        config = load_platform(platform_key)
        host = config.get("host", "")
        port = config.get("port", 22)
        username = self._session.username or "root"
        password = self._session.password

        if not host:
            self._log(f"ERROR: No host configured for {platform_key}. Check config/{platform_key}.json")
            return

        self._log(f"Copying {pub_path} -> {username}@{host}:/etc/safemon/pki/safemon.pub ...")

        self._scp_worker = Worker(
            self._do_copy_pubkey, str(pub_path), host, port, username, password
        )
        self._scp_worker.finished_ok.connect(self._on_copy_success)
        self._scp_worker.failed.connect(self._on_copy_failed)
        self._scp_worker.start()

    def _do_copy_pubkey(self, local_pub_path, host, port, username, password):
        mgr = SSHManager(host, port, username, password)
        mgr.connect()
        try:
            mgr.run_command("mkdir -p /etc/safemon/pki")
            mgr.put_file(local_pub_path, "/etc/safemon/pki/safemon.pub")
        finally:
            mgr.close()
        return "/etc/safemon/pki/safemon.pub"

    def _on_copy_success(self, remote_path):
        self._log(f"OK: Public key copied to device -> {remote_path}")

    def _on_copy_failed(self, error_text):
        self._log(f"ERROR: {error_text}")