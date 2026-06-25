"""
sign_verify_backend.py - Python backend for Sign/Verify page.
"""

from PyQt6.QtCore import QObject, pyqtSignal, pyqtSlot

from core.workers import Worker


class SignVerifyBackend(QObject):

    signSuccess   = pyqtSignal(str)
    signFailed    = pyqtSignal(str)
    verifySuccess = pyqtSignal(bool)
    verifyFailed  = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._sign_worker   = None
        self._verify_worker = None

    @pyqtSlot(str, str)
    def signFile(self, file_path, key_path):
        file_path = file_path.strip()
        key_path  = key_path.strip()

        if not file_path or not key_path:
            self.signFailed.emit("File and key must both be selected.")
            return

        self._sign_worker = Worker(self._do_sign, file_path, key_path)
        self._sign_worker.finished_ok.connect(self.signSuccess)
        self._sign_worker.failed.connect(self.signFailed)
        self._sign_worker.start()

    def _do_sign(self, file_path, key_path):
        from safemon_sign import load_key, hash_file, sign, save_signature
        priv, pub = load_key(key_path)
        if priv is None:
            raise ValueError("Key file does not contain a private key.")
        file_hash = hash_file(file_path)
        r, s      = sign(file_hash, priv)
        sig_path  = file_path + ".sig"
        save_signature(sig_path, r, s)
        return sig_path

    @pyqtSlot(str, str, str)
    def verifyFile(self, file_path, sig_path, pub_path):
        file_path = file_path.strip()
        sig_path  = sig_path.strip()
        pub_path  = pub_path.strip()

        if not file_path or not sig_path or not pub_path:
            self.verifyFailed.emit("File, signature, and public key must all be selected.")
            return

        self._verify_worker = Worker(self._do_verify, file_path, sig_path, pub_path)
        self._verify_worker.finished_ok.connect(self.verifySuccess)
        self._verify_worker.failed.connect(self.verifyFailed)
        self._verify_worker.start()

    def _do_verify(self, file_path, sig_path, pub_path):
        from safemon_sign import load_key, load_signature, hash_file, verify
        _, pub    = load_key(pub_path)
        r, s      = load_signature(sig_path)
        file_hash = hash_file(file_path)
        return verify(file_hash, r, s, pub)