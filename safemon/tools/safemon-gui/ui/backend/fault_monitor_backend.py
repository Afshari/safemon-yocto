"""
fault_monitor_backend.py - Python backend for Fault Monitor page.
"""

import grpc
from datetime import datetime

from PyQt6.QtCore import QObject, pyqtSignal, pyqtSlot, QThread

from core.config_manager import load_platform


class FaultStreamWorker(QThread):

    eventReceived = pyqtSignal(str, str, str, str)
    connected     = pyqtSignal()
    disconnected  = pyqtSignal()
    error         = pyqtSignal(str)

    def __init__(self, host, port):
        super().__init__()
        self._host    = host
        self._port    = port
        self._running = False

    def run(self):
        import fault_pb2
        import fault_pb2_grpc

        address      = f"{self._host}:{self._port}"
        self._running = True

        try:
            print(f"[fault-monitor] Attempting gRPC connection to {address}")
            channel = grpc.insecure_channel(address)
            stub    = fault_pb2_grpc.FaultServiceStub(channel)
            self.connected.emit()
            print(f"[fault-monitor] Connected successfully to {address}")

            for event in stub.StreamFaults(fault_pb2.StreamRequest()):
                if not self._running:
                    break
                ts        = datetime.fromtimestamp(event.timestamp / 1000.0)
                timestamp = ts.strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
                self.eventReceived.emit(
                    timestamp, event.status, event.source, event.signature
                )

        except grpc.RpcError as e:
            print(f"[fault-monitor] gRPC error: {e.code()} - {e.details()}")
            if e.code() == grpc.StatusCode.UNAVAILABLE:
                self.error.emit(
                    f"Could not connect to {address} - is safemon-app running? "
                    f"(code={e.code()}, details={e.details()})"
                )
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


class FaultMonitorBackend(QObject):

    eventReceived   = pyqtSignal(str, str, str, str)
    connected       = pyqtSignal()
    disconnected    = pyqtSignal()
    connectionError = pyqtSignal(str)

    def __init__(self, parent=None):
        super().__init__(parent)
        self._worker = None

    @pyqtSlot(str, int)
    def connectToDevice(self, platform_key, port):
        if self._worker and self._worker.isRunning():
            return

        try:
            from core.config_manager import load_platform
            config = load_platform(platform_key)
            host = config.get("host", "")
        except Exception as e:
            self.connectionError.emit(f"Config error: {e}")
            return

        if not host:
            self.connectionError.emit(f"No host configured for {platform_key}.")
            return

        self._worker = FaultStreamWorker(host, port)
        self._worker.eventReceived.connect(self.eventReceived)
        self._worker.connected.connect(self.connected)
        self._worker.disconnected.connect(self.disconnected)
        self._worker.error.connect(self.connectionError)
        self._worker.start()

    @pyqtSlot()
    def disconnectFromDevice(self):
        if self._worker:
            self._worker.stop()
            self._worker.quit()
            self._worker.wait(3000)
            if self._worker.isRunning():
                self._worker.terminate()
            self._worker = None
        self.disconnected.emit()

    @pyqtSlot()
    def isConnected(self):
        return self._worker is not None and self._worker.isRunning()
    
    @pyqtSlot(str, result=str)
    def hostForPlatform(self, platform_key):
        try:
            from core.config_manager import load_platform
            config = load_platform(platform_key)
            return config.get("host", "")
        except Exception:
            return ""