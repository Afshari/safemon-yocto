"""
device_status_backend.py - Python backend for Device Status page.
Exposed to QML via context property.
"""

from PyQt6.QtCore import QObject, pyqtSignal, pyqtSlot

from core.workers import Worker
from core.ssh_manager import SSHManager
from core.config_manager import load_platform


class DeviceStatusBackend(QObject):

    checkStarted   = pyqtSignal()
    checkFinished  = pyqtSignal()
    statusResult   = pyqtSignal(str, bool, str)
    connectionError = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._worker = None

    @pyqtSlot(str, str, str)
    def checkAll(self, platform_key, username, password):
        try:
            config = load_platform(platform_key)
        except Exception as e:
            self.connectionError.emit(f"Config error: {e}")
            return

        host = config.get("host", "")
        port = config.get("port", 22)

        if not host:
            self.connectionError.emit(f"No host configured for {platform_key}")
            return

        self.checkStarted.emit()

        self._worker = Worker(
            self._run_checks, host, port, username or "root", password
        )
        self._worker.finished_ok.connect(self._on_done)
        self._worker.failed.connect(self._on_failed)
        self._worker.start()

    def _run_checks(self, host, port, username, password):
        mgr = SSHManager(host, port, username, password)
        mgr.connect()
        results = {}
        try:
            results["safemon-app"]     = mgr.run_command("systemctl status safemon-app")
            results["safemon-display"] = mgr.run_command("systemctl status safemon-display")
            results["redis"]           = mgr.run_command("redis-cli ping")
            results["vcan"]            = mgr.run_command("ip link show vcan0 ; lsmod | grep vcan")
        finally:
            mgr.close()
        return results

    def _on_done(self, results):
        for key, (exit_code, out, err) in results.items():
            ok = self._evaluate(key, exit_code, out)
            output = out.strip()
            if err.strip():
                output += "\n" + err.strip()
            self.statusResult.emit(key, ok, output)
        self.checkFinished.emit()

    def _on_failed(self, error_text):
        self.connectionError.emit(error_text)
        self.checkFinished.emit()

    def _evaluate(self, key, exit_code, out):
        if key == "redis":
            return "PONG" in out
        if key == "vcan":
            return "vcan0" in out or "vcan" in out
        return exit_code == 0