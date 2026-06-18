"""
key_management.py - Key Management tab.
Handles key pair generation and copying public key to device via SCP.
"""

import os
from pathlib import Path

from PyQt6.QtWidgets import (
    QWidget, QHBoxLayout, QVBoxLayout, QLabel, QPushButton,
    QLineEdit, QFileDialog, QGroupBox, QComboBox, QTextEdit,
    QSizePolicy
)
from PyQt6.QtCore import Qt


class KeyManagementTab(QWidget):

    def __init__(self, parent=None):
        super().__init__(parent)
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
        panel.setStyleSheet("#leftPanel { background-color: #323232; }")

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

        self._target_combo = QComboBox()
        self._target_combo.addItems(["Raspberry Pi", "Jetson Orin Nano", "QEMU"])

        self._scp_btn = QPushButton("Copy Public Key to Device")
        self._scp_btn.setEnabled(False)
        self._scp_btn.setToolTip("SSH connection manager not yet available")

        scp_layout.addWidget(QLabel("Target device:"))
        scp_layout.addWidget(self._target_combo)
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
        pub_path = key_dir / "safemon.pub"

        try:
            from safemon_sign import generate_keypair
            generate_keypair(str(key_path))
            self._log(f"OK: Private key -> {key_path}")
            self._log(f"OK: Public key  -> {pub_path}")
        except ImportError:
            self._log("WARNING: safemon_sign module not found on path.")
            self._log(f"Would write private key to: {key_path}")
            self._log(f"Would write public key to:  {pub_path}")
        except Exception as e:
            self._log(f"ERROR: {e}")

    def _log(self, message: str):
        self._output.append(message)