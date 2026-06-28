import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

SAFEMON_SIGN_DIR = Path(__file__).resolve().parent.parent / "safemon-sign"
PROTO_DIR = Path(__file__).resolve().parent.parent.parent / "proto"
sys.path.insert(0, str(SAFEMON_SIGN_DIR))
sys.path.insert(0, str(PROTO_DIR))

from PyQt6.QtWidgets import QApplication
from PyQt6.QtQml import QQmlApplicationEngine
from PyQt6.QtGui import QIcon

from core.config_manager import ensure_all_configs

from ui.backend.device_status_backend import DeviceStatusBackend
from ui.backend.key_management_backend import KeyManagementBackend
from ui.backend.sign_verify_backend import SignVerifyBackend
from ui.backend.device_files_backend import DeviceFilesBackend
from ui.backend.fault_monitor_backend import FaultMonitorBackend
from ui.backend.settings_backend import SettingsBackend

def main():
    ensure_all_configs()

    os.environ["QT_QUICK_CONTROLS_STYLE"] = "Basic"

    app = QApplication(sys.argv)
    app.setApplicationName("safemon-gui")

    device_status_backend = DeviceStatusBackend()

    engine = QQmlApplicationEngine()
    engine.rootContext().setContextProperty("deviceStatusBackend", device_status_backend)

    key_management_backend = KeyManagementBackend()
    engine.rootContext().setContextProperty("keyManagementBackend", key_management_backend)

    sign_verify_backend = SignVerifyBackend()
    engine.rootContext().setContextProperty("signVerifyBackend", sign_verify_backend)

    device_files_backend = DeviceFilesBackend()
    engine.rootContext().setContextProperty("deviceFilesBackend", device_files_backend)

    fault_monitor_backend = FaultMonitorBackend()
    engine.rootContext().setContextProperty("faultMonitorBackend", fault_monitor_backend)

    settings_backend = SettingsBackend()
    engine.rootContext().setContextProperty("settingsBackend", settings_backend)

    qml_file = Path(__file__).resolve().parent / "ui" / "qml" / "main.qml"
    engine.load(str(qml_file))

    if not engine.rootObjects():
        print("ERROR: Failed to load QML file")
        sys.exit(1)

    sys.exit(app.exec())


if __name__ == "__main__":
    main()