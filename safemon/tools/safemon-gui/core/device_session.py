"""
device_session.py - Shared device connection state.
Holds the currently selected target platform, username, and password.
All tabs read from this instead of keeping their own copies.
"""

from PyQt6.QtCore import QObject, pyqtSignal


class DeviceSession(QObject):

    changed = pyqtSignal()

    def __init__(self):
        super().__init__()
        self._platform_key = "raspberry_pi"
        self._username = "root"
        self._password = ""

    @property
    def platform_key(self):
        return self._platform_key

    @platform_key.setter
    def platform_key(self, value):
        self._platform_key = value
        self.changed.emit()

    @property
    def username(self):
        return self._username

    @username.setter
    def username(self, value):
        self._username = value
        self.changed.emit()

    @property
    def password(self):
        return self._password

    @password.setter
    def password(self, value):
        self._password = value
        self.changed.emit()