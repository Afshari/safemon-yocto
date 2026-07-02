"""
key_management_backend.py - Python backend for Key Management page.
"""

import posixpath
from pathlib import Path

from PyQt6.QtCore import QObject, pyqtSignal, pyqtSlot

from core.workers import Worker
from core.ssh_manager import SSHManager
from core.config_manager import load_platform


class KeyManagementBackend(QObject):

    keyGenSuccess = pyqtSignal(str, str)
    keyGenFailed  = pyqtSignal(str)
    scpSuccess    = pyqtSignal(str)
    scpFailed     = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._worker = None

    @pyqtSlot(str)
    def generateKeys(self, key_dir):
        key_dir = key_dir.strip()
        if not key_dir:
            self.keyGenFailed.emit("No output directory specified.")
            return

        try:
            Path(key_dir).mkdir(parents=True, exist_ok=True)
        except Exception as e:
            self.keyGenFailed.emit(f"Could not create directory: {e}")
            return

        key_path = Path(key_dir) / "safemon.key"
        self._worker = Worker(self._do_generate, key_path)
        self._worker.finished_ok.connect(
            lambda r: self.keyGenSuccess.emit(str(r[0]), str(r[1]))
        )
        self._worker.failed.connect(self.keyGenFailed)
        self._worker.start()

    def _do_generate(self, key_path):
        from safemon_sign import random_scalar, G, save_keypair, save_pubkey
        private_key = random_scalar()
        public_key  = private_key * G
        pub_path    = key_path.with_suffix(".pub")
        save_keypair(str(key_path), private_key, public_key)
        save_pubkey(str(pub_path), public_key)
        return key_path, pub_path

    @pyqtSlot(str, str, str, str)
    def copyPublicKey(self, pub_path, platform_key, username, password):
        pub_path = pub_path.strip()
        if not pub_path:
            self.scpFailed.emit("No public key file selected.")
            return
        if not Path(pub_path).exists():
            self.scpFailed.emit(f"File not found: {pub_path}")
            return

        try:
            config  = load_platform(platform_key)
        except Exception as e:
            self.scpFailed.emit(f"Config error: {e}")
            return

        host = config.get("host", "")
        port = config.get("port", 22)

        if not host:
            self.scpFailed.emit(f"No host configured for {platform_key}.")
            return

        self._scp_worker = Worker(
            self._do_copy, pub_path, host, port, username or "root", password
        )
        self._scp_worker.finished_ok.connect(lambda r: self.scpSuccess.emit(str(r)))
        self._scp_worker.failed.connect(self.scpFailed)
        self._scp_worker.start()

    def _do_copy(self, pub_path, host, port, username, password):
        dest      = "/etc/safemon/pki/safemon.pub"
        remote_dir = posixpath.dirname(dest)
        mgr = SSHManager(host, port, username, password)
        mgr.connect()
        try:
            mgr.run_command(f"mkdir -p {remote_dir}")
            mgr.put_file(pub_path, dest)
        finally:
            mgr.close()
        return dest

    @pyqtSlot(result=str)
    def defaultKeyDir(self):
        from pathlib import Path
        return str(Path.home() / ".safemon")