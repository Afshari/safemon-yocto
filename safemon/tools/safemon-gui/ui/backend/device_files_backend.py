"""
device_files_backend.py - Python backend for Device Files page.
"""

import os
import posixpath
import tempfile
from pathlib import Path

from PyQt6.QtCore import QObject, pyqtSignal, pyqtSlot

from core.workers import Worker
from core.ssh_manager import SSHManager
from core.config_manager import load_platform, load_device_files


class DeviceFilesBackend(QObject):

    knownFilesLoaded  = pyqtSignal(list)
    fileLoaded        = pyqtSignal(str, str)
    copySuccess       = pyqtSignal(str)
    copyFailed        = pyqtSignal(str)
    loadFailed        = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._files = {}
        self._worker = None

    @pyqtSlot()
    def loadKnownFiles(self):
        try:
            self._files = load_device_files()
            self.knownFilesLoaded.emit(list(self._files.keys()))
        except Exception as e:
            self.loadFailed.emit(str(e))

    @pyqtSlot(str)
    def devicePathForFile(self, friendly_name):
        return self._files.get(friendly_name, "")

    @pyqtSlot(str, str, str, str)
    def loadFromDevice(self, friendly_name, platform_key, username, password):
        device_path = self._files.get(friendly_name, "")
        if not device_path:
            self.loadFailed.emit(f"No device path for: {friendly_name}")
            return

        try:
            config = load_platform(platform_key)
        except Exception as e:
            self.loadFailed.emit(str(e))
            return

        host = config.get("host", "")
        port = config.get("port", 22)
        if not host:
            self.loadFailed.emit(f"No host configured for {platform_key}.")
            return

        self._worker = Worker(
            self._do_load, device_path, host, port, username or "root", password
        )
        self._worker.finished_ok.connect(
            lambda content: self.fileLoaded.emit(friendly_name, content)
        )
        self._worker.failed.connect(self.loadFailed)
        self._worker.start()

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

    @pyqtSlot(str, str, str, str, str)
    def copyKnownToDevice(self, friendly_name, local_path,
                          platform_key, username, password):
        device_path = self._files.get(friendly_name, "")
        if not device_path:
            self.copyFailed.emit(f"No device path for: {friendly_name}")
            return
        if not local_path or not Path(local_path).exists():
            self.copyFailed.emit(f"Local file not found: {local_path}")
            return

        try:
            config = load_platform(platform_key)
        except Exception as e:
            self.copyFailed.emit(str(e))
            return

        host = config.get("host", "")
        port = config.get("port", 22)
        if not host:
            self.copyFailed.emit(f"No host configured for {platform_key}.")
            return

        self._copy_worker = Worker(
            self._do_copy, local_path, device_path, host, port,
            username or "root", password
        )
        self._copy_worker.finished_ok.connect(lambda r: self.copySuccess.emit(str(r)))
        self._copy_worker.failed.connect(self.copyFailed)
        self._copy_worker.start()

    @pyqtSlot(str, str, str, str, str)
    def copyFileToDevice(self, local_path, dest_path,
                         platform_key, username, password):
        if not local_path or not Path(local_path).exists():
            self.copyFailed.emit(f"Local file not found: {local_path}")
            return

        filename = Path(local_path).name
        if dest_path.endswith("/"):
            dest_path = dest_path + filename
        elif "." not in posixpath.basename(dest_path):
            dest_path = dest_path + "/" + filename

        try:
            config = load_platform(platform_key)
        except Exception as e:
            self.copyFailed.emit(str(e))
            return

        host = config.get("host", "")
        port = config.get("port", 22)
        if not host:
            self.copyFailed.emit(f"No host configured for {platform_key}.")
            return

        self._transfer_worker = Worker(
            self._do_copy, local_path, dest_path, host, port,
            username or "root", password
        )
        self._transfer_worker.finished_ok.connect(lambda r: self.copySuccess.emit(str(r)))
        self._transfer_worker.failed.connect(self.copyFailed)
        self._transfer_worker.start()

    def _do_copy(self, local_path, device_path, host, port, username, password):
        mgr = SSHManager(host, port, username, password)
        mgr.connect()
        try:
            remote_dir = posixpath.dirname(device_path)
            mgr.run_command(f"mkdir -p {remote_dir}")
            mgr.put_file(local_path, device_path)
        finally:
            mgr.close()
        return device_path