"""
device_status.py - Device Status tab.
Runs health checks over SSH: systemd services, Redis, virtual CAN.
"""

from PyQt6.QtWidgets import (
    QWidget, QHBoxLayout, QVBoxLayout, QLabel, QPushButton,
    QLineEdit, QGroupBox, QComboBox, QTextEdit
)
from PyQt6.QtCore import Qt

from core.workers import Worker
from core.ssh_manager import SSHManager
from core.config_manager import load_platform


PLATFORM_KEYS = {
    "Raspberry Pi": "raspberry_pi",
    "Jetson Orin Nano": "jetson_orin_nano",
    "QEMU": "qemu",
}

PLATFORM_DISPLAY_NAMES_REVERSE = {v: k for k, v in PLATFORM_KEYS.items()}

class DeviceStatusTab(QWidget):

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

        check_btn = QPushButton("Check All")
        check_btn.clicked.connect(self._check_all)

        # --- Status indicators ---
        status_group = QGroupBox("Status")
        status_layout = QVBoxLayout(status_group)
        status_layout.setSpacing(6)

        self._status_labels = {}
        for key in ("safemon-app", "safemon-display", "redis", "vcan"):
            lbl = QLabel(f"\u25CF  {key}")
            lbl.setStyleSheet("color: grey;")
            status_layout.addWidget(lbl)
            self._status_labels[key] = lbl

        layout.addWidget(check_btn)
        layout.addSpacing(8)
        layout.addWidget(status_group)
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

        header = QLabel("Check Results")
        header.setStyleSheet("font-size: 14px; color: grey;")

        self._output = QTextEdit()
        self._output.setReadOnly(True)
        self._output.setPlaceholderText("Run 'Check All' to see results here...")

        layout.addWidget(header)
        layout.addWidget(self._output)

        return panel

    # ------------------------------------------------------------------
    # Check All
    # ------------------------------------------------------------------

    def _check_all(self):
        platform_key = self._session.platform_key
        target_name = PLATFORM_DISPLAY_NAMES_REVERSE.get(platform_key, platform_key)
        config = load_platform(platform_key)

        host = config.get("host", "")
        port = config.get("port", 22)
        username = self._session.username or "root"
        password = self._session.password

        if not host:
            self._log(f"ERROR: No host configured for {target_name}. Check config/{platform_key}.json")
            return

        self._output.clear()
        self._set_all_status_pending()
        self._log(f"Connecting to {target_name} ({host}:{port}) as {username}...")

        self._check_worker = Worker(self._run_checks, host, port, username, password)
        self._check_worker.finished_ok.connect(self._on_checks_done)
        self._check_worker.failed.connect(self._on_checks_failed)
        self._check_worker.start()

    def _run_checks(self, host, port, username, password):
        mgr = SSHManager(host, port, username, password)
        mgr.connect()

        results = {}
        try:
            results["safemon-app"] = mgr.run_command("systemctl status safemon-app")
            results["safemon-display"] = mgr.run_command("systemctl status safemon-display")
            results["redis"] = mgr.run_command("redis-cli ping")
            results["vcan"] = mgr.run_command("ip link show vcan0 ; lsmod | grep vcan")
        finally:
            mgr.close()

        return results

    def _on_checks_done(self, results):
        for key, (exit_code, out, err) in results.items():
            self._log(f"--- {key} ---")
            if out.strip():
                self._log(out.strip())
            if err.strip():
                self._log(err.strip())
            self._log("")

            ok = self._evaluate(key, exit_code, out)
            self._set_status(key, ok)

        self._log("Check complete.")

    def _on_checks_failed(self, error_text):
        self._log(f"ERROR: {error_text}")
        self._set_all_status_error()

    def _evaluate(self, key, exit_code, out):
        if key == "redis":
            return "PONG" in out
        if key == "vcan":
            return "vcan0" in out or "vcan" in out
        # systemd services
        return exit_code == 0

    # ------------------------------------------------------------------
    # Status indicator helpers
    # ------------------------------------------------------------------

    def _set_status(self, key, ok):
        lbl = self._status_labels[key]
        color = "#4caf50" if ok else "#c62828"
        lbl.setStyleSheet(f"color: {color};")

    def _set_all_status_pending(self):
        for lbl in self._status_labels.values():
            lbl.setStyleSheet("color: grey;")

    def _set_all_status_error(self):
        for lbl in self._status_labels.values():
            lbl.setStyleSheet("color: #c62828;")

    def _log(self, message: str):
        self._output.append(message)