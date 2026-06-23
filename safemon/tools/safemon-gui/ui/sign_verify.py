"""
sign_verify.py - Sign / Verify tab.
Signs files with a private key and verifies signatures against a public key.
"""

import sys
from pathlib import Path

from PyQt6.QtWidgets import (
    QWidget, QHBoxLayout, QVBoxLayout, QLabel, QPushButton,
    QLineEdit, QFileDialog, QGroupBox, QTextEdit
)
from PyQt6.QtCore import Qt

from core.workers import Worker


class SignVerifyTab(QWidget):

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
        panel.setStyleSheet("#leftPanel { background-color: #4a4a4a; }")

        layout = QVBoxLayout(panel)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(12)

        # --- Sign section ---
        sign_group = QGroupBox("Sign File")
        sign_layout = QVBoxLayout(sign_group)
        sign_layout.setSpacing(6)

        self._sign_file_field = QLineEdit()
        self._sign_file_field.setPlaceholderText("File to sign")
        sign_file_btn = QPushButton("Choose File")
        sign_file_btn.clicked.connect(self._browse_sign_file)

        self._sign_key_field = QLineEdit()
        self._sign_key_field.setPlaceholderText("Private key (.key)")
        sign_key_btn = QPushButton("Choose Key")
        sign_key_btn.clicked.connect(self._browse_sign_key)

        sign_btn = QPushButton("Sign")
        sign_btn.clicked.connect(self._do_sign)

        sign_layout.addWidget(self._sign_file_field)
        sign_layout.addWidget(sign_file_btn)
        sign_layout.addWidget(self._sign_key_field)
        sign_layout.addWidget(sign_key_btn)
        sign_layout.addWidget(sign_btn)

        # --- Verify section ---
        verify_group = QGroupBox("Verify File")
        verify_layout = QVBoxLayout(verify_group)
        verify_layout.setSpacing(6)

        self._verify_file_field = QLineEdit()
        self._verify_file_field.setPlaceholderText("File to verify")
        verify_file_btn = QPushButton("Choose File")
        verify_file_btn.clicked.connect(self._browse_verify_file)

        self._verify_pub_field = QLineEdit()
        self._verify_pub_field.setPlaceholderText("Public key (.pub)")
        verify_pub_btn = QPushButton("Choose Public Key")
        verify_pub_btn.clicked.connect(self._browse_verify_pub)

        self._verify_sig_field = QLineEdit()
        self._verify_sig_field.setPlaceholderText("Signature (.sig)")
        verify_sig_btn = QPushButton("Choose Signature")
        verify_sig_btn.clicked.connect(self._browse_verify_sig)

        verify_btn = QPushButton("Verify")
        verify_btn.clicked.connect(self._do_verify)

        verify_layout.addWidget(self._verify_file_field)
        verify_layout.addWidget(verify_file_btn)
        verify_layout.addWidget(self._verify_pub_field)
        verify_layout.addWidget(verify_pub_btn)
        verify_layout.addWidget(self._verify_sig_field)
        verify_layout.addWidget(verify_sig_btn)
        verify_layout.addWidget(verify_btn)

        layout.addWidget(sign_group)
        layout.addWidget(verify_group)
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

        header = QLabel("Result")
        header.setStyleSheet("font-size: 14px; color: grey;")

        self._result_label = QLabel("")
        self._result_label.setStyleSheet("font-size: 18px; font-weight: bold;")

        self._output = QTextEdit()
        self._output.setReadOnly(True)
        self._output.setPlaceholderText("Sign/verify results will appear here...")

        layout.addWidget(header)
        layout.addWidget(self._result_label)
        layout.addWidget(self._output)

        return panel

    # ------------------------------------------------------------------
    # Browse handlers
    # ------------------------------------------------------------------

    def _browse_sign_file(self):
        path, _ = QFileDialog.getOpenFileName(self, "Select File to Sign")
        if path:
            self._sign_file_field.setText(path)

    def _browse_sign_key(self):
        path, _ = QFileDialog.getOpenFileName(self, "Select Private Key", filter="Key files (*.key)")
        if path:
            self._sign_key_field.setText(path)

    def _browse_verify_file(self):
        path, _ = QFileDialog.getOpenFileName(self, "Select File to Verify")
        if path:
            self._verify_file_field.setText(path)

    def _browse_verify_pub(self):
        path, _ = QFileDialog.getOpenFileName(self, "Select Public Key", filter="Pub files (*.pub)")
        if path:
            self._verify_pub_field.setText(path)

    def _browse_verify_sig(self):
        path, _ = QFileDialog.getOpenFileName(self, "Select Signature File", filter="Signature files (*.sig)")
        if path:
            self._verify_sig_field.setText(path)

    # ------------------------------------------------------------------
    # Sign
    # ------------------------------------------------------------------

    def _do_sign(self):
        file_path = self._sign_file_field.text().strip()
        key_path = self._sign_key_field.text().strip()

        if not file_path or not key_path:
            self._log("ERROR: File and key must both be selected.")
            return

        self._result_label.setText("")
        self._sign_worker = Worker(self._sign_task, file_path, key_path)
        self._sign_worker.finished_ok.connect(self._on_sign_success)
        self._sign_worker.failed.connect(self._on_sign_failed)
        self._sign_worker.start()

    def _sign_task(self, file_path, key_path):
        from safemon_sign import load_key, hash_file, sign, save_signature

        priv, pub = load_key(key_path)
        if priv is None:
            raise ValueError("Key file does not contain a private key.")

        file_hash = hash_file(file_path)
        r, s = sign(file_hash, priv)
        sig_path = file_path + ".sig"
        save_signature(sig_path, r, s)
        return sig_path

    def _on_sign_success(self, sig_path):
        self._log(f"OK: Signature written -> {sig_path}")
        self._result_label.setText("SIGNED")
        self._result_label.setStyleSheet("font-size: 18px; font-weight: bold; color: #4caf50;")

    def _on_sign_failed(self, error_text):
        self._log(f"ERROR: {error_text}")
        self._result_label.setText("SIGN FAILED")
        self._result_label.setStyleSheet("font-size: 18px; font-weight: bold; color: #c62828;")

    # ------------------------------------------------------------------
    # Verify
    # ------------------------------------------------------------------

    def _do_verify(self):
        file_path = self._verify_file_field.text().strip()
        pub_path = self._verify_pub_field.text().strip()
        sig_path = self._verify_sig_field.text().strip()

        if not file_path or not pub_path or not sig_path:
            self._log("ERROR: File, public key, and signature must all be selected.")
            return

        self._result_label.setText("")
        self._verify_worker = Worker(self._verify_task, file_path, pub_path, sig_path)
        self._verify_worker.finished_ok.connect(self._on_verify_success)
        self._verify_worker.failed.connect(self._on_verify_failed)
        self._verify_worker.start()

    def _verify_task(self, file_path, pub_path, sig_path):
        from safemon_sign import load_key, load_signature, hash_file, verify

        _, pub = load_key(pub_path)
        r, s = load_signature(sig_path)
        file_hash = hash_file(file_path)
        is_valid = verify(file_hash, r, s, pub)
        return is_valid

    def _on_verify_success(self, is_valid):
        if is_valid:
            self._log("OK: Signature is VALID.")
            self._result_label.setText("VALID")
            self._result_label.setStyleSheet("font-size: 18px; font-weight: bold; color: #4caf50;")
        else:
            self._log("FAIL: Signature is INVALID.")
            self._result_label.setText("INVALID")
            self._result_label.setStyleSheet("font-size: 18px; font-weight: bold; color: #c62828;")

    def _on_verify_failed(self, error_text):
        self._log(f"ERROR: {error_text}")
        self._result_label.setText("VERIFY FAILED")
        self._result_label.setStyleSheet("font-size: 18px; font-weight: bold; color: #c62828;")

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _log(self, message: str):
        self._output.append(message)