"""
settings_backend.py - Backend for the Settings/Config editor page.
Handles loading, saving, and resetting all platform and app configs.
"""

from PyQt6.QtCore import QObject, pyqtSignal, pyqtSlot

from core.config_manager import (
    load_platform, save_platform, load_app, save,
    DEFAULT_RASPBERRY_PI, DEFAULT_JETSON_ORIN_NANO,
    DEFAULT_QEMU, DEFAULT_APP
)


class SettingsBackend(QObject):

    configLoaded  = pyqtSignal(str, str)
    saveSuccess   = pyqtSignal(str)
    saveFailed    = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)

    # ------------------------------------------------------------------
    # Load
    # ------------------------------------------------------------------

    @pyqtSlot()
    def loadAll(self):
        import json
        for platform_key in ["raspberry_pi", "jetson_orin_nano", "qemu"]:
            try:
                data = load_platform(platform_key)
                self.configLoaded.emit(platform_key, json.dumps(data))
            except Exception as e:
                self.saveFailed.emit(f"Failed to load {platform_key}: {e}")

        try:
            data = load_app()
            self.configLoaded.emit("app", json.dumps(data))
        except Exception as e:
            self.saveFailed.emit(f"Failed to load app config: {e}")

    # ------------------------------------------------------------------
    # Save platform
    # ------------------------------------------------------------------

    @pyqtSlot(str, str, int, str)
    def savePlatform(self, platform_key, host, port, user):
        try:
            existing = load_platform(platform_key)
            existing["host"] = host.strip()
            existing["port"] = port
            existing["user"] = user.strip()
            save_platform(platform_key, existing)
            self.saveSuccess.emit(f"Saved {platform_key} config.")
        except Exception as e:
            self.saveFailed.emit(f"Failed to save {platform_key}: {e}")

    # ------------------------------------------------------------------
    # Reset platform
    # ------------------------------------------------------------------

    @pyqtSlot(str, result=str)
    def defaultsForPlatform(self, platform_key):
        import json
        defaults = {
            "raspberry_pi":     DEFAULT_RASPBERRY_PI,
            "jetson_orin_nano": DEFAULT_JETSON_ORIN_NANO,
            "qemu":             DEFAULT_QEMU,
        }
        return json.dumps(defaults.get(platform_key, {}))

    @pyqtSlot(str)
    def resetPlatform(self, platform_key):
        defaults = {
            "raspberry_pi":     DEFAULT_RASPBERRY_PI,
            "jetson_orin_nano": DEFAULT_JETSON_ORIN_NANO,
            "qemu":             DEFAULT_QEMU,
        }
        try:
            save_platform(platform_key, defaults[platform_key])
            self.saveSuccess.emit(f"Reset {platform_key} to defaults.")
        except Exception as e:
            self.saveFailed.emit(f"Failed to reset {platform_key}: {e}")

    # ------------------------------------------------------------------
    # Save app config
    # ------------------------------------------------------------------

    @pyqtSlot(int, int, str)
    def saveApp(self, journal_lines, journal_refresh_seconds, key_dir):
        try:
            existing = load_app()
            existing["journal_lines"]            = journal_lines
            existing["journal_refresh_seconds"]  = journal_refresh_seconds
            existing["key_dir"]                  = key_dir.strip()
            save("app.json", existing)
            self.saveSuccess.emit("Saved app config.")
        except Exception as e:
            self.saveFailed.emit(f"Failed to save app config: {e}")

    # ------------------------------------------------------------------
    # Reset app config
    # ------------------------------------------------------------------

    @pyqtSlot()
    def resetApp(self):
        try:
            save("app.json", DEFAULT_APP)
            self.saveSuccess.emit("Reset app config to defaults.")
        except Exception as e:
            self.saveFailed.emit(f"Failed to reset app config: {e}")