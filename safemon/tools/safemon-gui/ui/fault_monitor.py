"""
fault_monitor.py - Fault Monitor tab.
Streams live fault events from safemon-app via gRPC and displays
them in a color-coded table with auto-scroll.
"""

import grpc
from datetime import datetime

from PyQt6.QtWidgets import (
    QWidget, QHBoxLayout, QVBoxLayout, QLabel, QPushButton,
    QLineEdit, QGroupBox, QTableWidget, QTableWidgetItem,
    QHeaderView, QSizePolicy
)
from PyQt6.QtCore import Qt, QThread, pyqtSignal
from PyQt6.QtGui import QColor

from core.config_manager import load_platform


STATUS_COLORS = {
    "OK":    QColor("#1b5e20"),
    "WARN":  QColor("#e65100"),
    "ERROR": QColor("#b71c1c"),
}

TEXT_COLORS = {
    "OK":    QColor("#a5d6a7"),
    "WARN":  QColor("#ffcc80"),
    "ERROR": QColor("#ef9a9a"),
}


class FaultStreamWorker(QThread):
    """
    Runs the gRPC stream in a background thread.
    Emits one signal per fault event, plus signals for
    connection state changes and errors.
    """

    event_received = pyqtSignal(str, str, str, str)
    connected      = pyqtSignal()
    disconnected   = pyqtSignal()
    error          = pyqtSignal(str)

    def __init__(self, host, port):
        super().__init__()
        self._host = host
        self._port = port
        self._running = False

    def run(self):
        import fault_pb2
        import fault_pb2_grpc

        address = f"{self._host}:{self._port}"
        self._running = True

        try:
            channel = grpc.insecure_channel(address)
            stub = fault_pb2_grpc.FaultServiceStub(channel)
            self.connected.emit()

            for event in stub.StreamFaults(fault_pb2.StreamRequest()):
                if not self._running:
                    break
                ts = datetime.fromtimestamp(event.timestamp / 1000.0)
                timestamp = ts.strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
                self.event_received.emit(timestamp, event.status, event.source, event.signature)

        except grpc.RpcError as e:
            if e.code() == grpc.StatusCode.UNAVAILABLE:
                self.error.emit(f"Could not connect to {address} - is safemon-app running?")
            elif e.code() == grpc.StatusCode.CANCELLED:
                pass
            else:
                self.error.emit(f"gRPC error: {e.code()} - {e.details()}")
        except Exception as e:
            self.error.emit(str(e))
        finally:
            self._running = False
            self.disconnected.emit()

    def stop(self):
        self._running = False


class FaultMonitorTab(QWidget):

    def __init__(self, session, parent=None):
        super().__init__(parent)
        self._session = session
        self._worker = None
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

        conn_group = QGroupBox("Connection")
        conn_layout = QVBoxLayout(conn_group)
        conn_layout.setSpacing(6)

        self._host_field = QLineEdit()
        self._host_field.setPlaceholderText("Host (IP or hostname)")

        self._port_field = QLineEdit("50051")
        self._port_field.setPlaceholderText("gRPC port")

        conn_layout.addWidget(QLabel("Host:"))
        conn_layout.addWidget(self._host_field)
        conn_layout.addWidget(QLabel("Port:"))
        conn_layout.addWidget(self._port_field)

        self._connect_btn = QPushButton("Connect")
        self._connect_btn.clicked.connect(self._toggle_connection)

        self._status_label = QLabel("Disconnected")
        self._status_label.setStyleSheet("color: grey;")
        self._status_label.setAlignment(Qt.AlignmentFlag.AlignCenter)

        clear_btn = QPushButton("Clear Table")
        clear_btn.clicked.connect(self._clear_table)

        layout.addWidget(conn_group)
        layout.addWidget(self._connect_btn)
        layout.addWidget(self._status_label)
        layout.addSpacing(8)
        layout.addWidget(clear_btn)
        layout.addStretch()

        self._session.changed.connect(self._on_session_changed)
        self._on_session_changed()

        return panel

    # ------------------------------------------------------------------
    # Right panel
    # ------------------------------------------------------------------

    def _build_right_panel(self):
        panel = QWidget()
        layout = QVBoxLayout(panel)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.setSpacing(8)

        header = QLabel("Fault Events")
        header.setStyleSheet("font-size: 14px; color: grey;")

        self._table = QTableWidget()
        self._table.setColumnCount(4)
        self._table.setHorizontalHeaderLabels(["Timestamp", "Status", "Source", "Signature"])
        self._table.horizontalHeader().setSectionResizeMode(0, QHeaderView.ResizeMode.ResizeToContents)
        self._table.horizontalHeader().setSectionResizeMode(1, QHeaderView.ResizeMode.Stretch)
        self._table.horizontalHeader().setSectionResizeMode(3, QHeaderView.ResizeMode.ResizeToContents)
        self._table.setEditTriggers(QTableWidget.EditTrigger.NoEditTriggers)
        self._table.setSelectionBehavior(QTableWidget.SelectionBehavior.SelectRows)
        self._table.setAlternatingRowColors(True)
        self._table.verticalHeader().setVisible(False)

        layout.addWidget(header)
        layout.addWidget(self._table)

        return panel

    # ------------------------------------------------------------------
    # Session sync
    # ------------------------------------------------------------------

    def _on_session_changed(self):
        if self._worker and self._worker.isRunning():
            return
        try:
            config = load_platform(self._session.platform_key)
            host = config.get("host", "")
            self._host_field.setText(host)
        except Exception:
            pass

    # ------------------------------------------------------------------
    # Connect / Disconnect
    # ------------------------------------------------------------------

    def _toggle_connection(self):
        if self._worker and self._worker.isRunning():
            self._disconnect()
        else:
            self._connect()

    def _connect(self):
        host = self._host_field.text().strip()
        port_text = self._port_field.text().strip()

        if not host:
            self._set_status("ERROR: No host specified.", "error")
            return

        try:
            port = int(port_text)
        except ValueError:
            self._set_status("ERROR: Invalid port number.", "error")
            return

        self._worker = FaultStreamWorker(host, port)
        self._worker.event_received.connect(self._on_event)
        self._worker.connected.connect(self._on_connected)
        self._worker.disconnected.connect(self._on_disconnected)
        self._worker.error.connect(self._on_error)
        self._worker.start()

        self._connect_btn.setText("Disconnect")
        self._set_status(f"Connecting to {host}:{port}...", "pending")

    def _disconnect(self):
        if self._worker:
            self._worker.stop()
            self._worker.quit()
            self._worker.wait()
        self._connect_btn.setText("Connect")
        self._set_status("Disconnected", "grey")

    # ------------------------------------------------------------------
    # Worker signals
    # ------------------------------------------------------------------

    def _on_connected(self):
        self._set_status("Connected", "ok")

    def _on_disconnected(self):
        self._connect_btn.setText("Connect")
        self._set_status("Disconnected", "grey")

    def _on_error(self, message):
        self._set_status(f"ERROR: {message}", "error")
        self._connect_btn.setText("Connect")

    def _on_event(self, timestamp, status, source, signature):
        row = self._table.rowCount()
        self._table.insertRow(row)

        for col, text in enumerate([timestamp, status, source, signature]):
            item = QTableWidgetItem(text)
            item.setTextAlignment(Qt.AlignmentFlag.AlignVCenter | Qt.AlignmentFlag.AlignLeft)

            severity = self._severity(status)
            item.setBackground(STATUS_COLORS.get(severity, QColor("#2b2b2b")))
            item.setForeground(TEXT_COLORS.get(severity, QColor("#e0e0e0")))

            self._table.setItem(row, col, item)

        self._table.scrollToBottom()

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _severity(self, status):
        if status.startswith("OK"):
            return "OK"
        if status.startswith("WARN"):
            return "WARN"
        if status.startswith("ERROR"):
            return "ERROR"
        return ""

    def _clear_table(self):
        self._table.setRowCount(0)

    def _set_status(self, message, level):
        colors = {
            "ok":      "#4caf50",
            "error":   "#c62828",
            "pending": "#ffcc80",
            "grey":    "grey",
        }
        color = colors.get(level, "grey")
        self._status_label.setText(message)
        self._status_label.setStyleSheet(f"color: {color};")