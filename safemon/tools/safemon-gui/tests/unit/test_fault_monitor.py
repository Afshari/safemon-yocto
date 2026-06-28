"""
test_fault_monitor.py - Unit tests for fault monitor logic.
No real gRPC connections, no real threads started.
Tests focus on pure logic: timestamp formatting, status
classification, and signal wiring.
"""

import time
import pytest
from datetime import datetime
from unittest.mock import MagicMock, patch


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def make_fault_event(status, timestamp_ms=None, source="vcan0", signature=""):
    event = MagicMock()
    event.status    = status
    event.timestamp = timestamp_ms or int(time.time() * 1000)
    event.source    = source
    event.signature = signature
    return event


# ---------------------------------------------------------------------------
# Timestamp formatting
# ---------------------------------------------------------------------------

class TestTimestampFormatting:

    def _format(self, ms):
        from datetime import datetime
        dt = datetime.fromtimestamp(ms / 1000.0)
        return dt.strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]

    def test_timestamp_is_string(self):
        ts = self._format(1718870400000)
        assert isinstance(ts, str)

    def test_timestamp_contains_date_separator(self):
        ts = self._format(1718870400000)
        assert "-" in ts

    def test_timestamp_contains_time_separator(self):
        ts = self._format(1718870400000)
        assert ":" in ts

    def test_timestamp_contains_milliseconds(self):
        ts = self._format(1718870400123)
        assert "." in ts

    def test_timestamp_format_length(self):
        ts = self._format(1718870400000)
        assert len(ts) == 23

    def test_timestamp_zero_ms(self):
        ts = self._format(0)
        assert isinstance(ts, str)
        assert len(ts) == 23

    def test_timestamp_different_values_differ(self):
        ts1 = self._format(1718870400000)
        ts2 = self._format(1718870461000)
        assert ts1 != ts2


# ---------------------------------------------------------------------------
# Status classification
# ---------------------------------------------------------------------------

class TestStatusClassification:

    def _severity(self, status):
        if status.startswith("OK"):
            return "OK"
        if status.startswith("WARN"):
            return "WARN"
        if status.startswith("ERROR"):
            return "ERROR"
        return ""

    def test_ok_no_fault(self):
        assert self._severity("OK:NO_FAULT") == "OK"

    def test_warn_unknown_id(self):
        assert self._severity("WARN:UNKNOWN_ID:0x123") == "WARN"

    def test_error_timeout_no_new_frame(self):
        assert self._severity("ERROR:TIMEOUT:no new frame") == "ERROR"

    def test_error_timeout_no_frame_received(self):
        assert self._severity("ERROR:TIMEOUT:no frame received") == "ERROR"

    def test_unknown_status_returns_empty(self):
        assert self._severity("UNKNOWN:SOMETHING") == ""

    def test_empty_status_returns_empty(self):
        assert self._severity("") == ""

    def test_case_sensitive_ok(self):
        assert self._severity("ok:no_fault") == ""

    def test_case_sensitive_warn(self):
        assert self._severity("warn:something") == ""

    def test_case_sensitive_error(self):
        assert self._severity("error:something") == ""

    @pytest.mark.parametrize("status,expected", [
        ("OK:NO_FAULT",                     "OK"),
        ("WARN:UNKNOWN_ID:0x123",           "WARN"),
        ("WARN:UNKNOWN_ID:0xABC",           "WARN"),
        ("ERROR:TIMEOUT:no new frame",      "ERROR"),
        ("ERROR:TIMEOUT:no frame received", "ERROR"),
        ("",                                ""),
        ("UNKNOWN",                         ""),
    ])
    def test_parametrized_classification(self, status, expected):
        assert self._severity(status) == expected


# ---------------------------------------------------------------------------
# FaultStreamWorker - signal wiring (no thread started)
# ---------------------------------------------------------------------------

class TestFaultStreamWorkerSignals:

    def test_worker_has_event_received_signal(self):
        from ui.backend.fault_monitor_backend import FaultStreamWorker
        worker = FaultStreamWorker("192.168.1.1", 50051)
        assert hasattr(worker, "eventReceived")

    def test_worker_has_connected_signal(self):
        from ui.backend.fault_monitor_backend import FaultStreamWorker
        worker = FaultStreamWorker("192.168.1.1", 50051)
        assert hasattr(worker, "connected")

    def test_worker_has_disconnected_signal(self):
        from ui.backend.fault_monitor_backend import FaultStreamWorker
        worker = FaultStreamWorker("192.168.1.1", 50051)
        assert hasattr(worker, "disconnected")

    def test_worker_has_error_signal(self):
        from ui.backend.fault_monitor_backend import FaultStreamWorker
        worker = FaultStreamWorker("192.168.1.1", 50051)
        assert hasattr(worker, "error")

    def test_worker_has_stop_method(self):
        from ui.backend.fault_monitor_backend import FaultStreamWorker
        worker = FaultStreamWorker("192.168.1.1", 50051)
        assert callable(worker.stop)

    def test_stop_sets_running_false(self):
        from ui.backend.fault_monitor_backend import FaultStreamWorker
        worker = FaultStreamWorker("192.168.1.1", 50051)
        worker._running = True
        worker.stop()
        assert worker._running is False

    def test_worker_stores_host(self):
        from ui.backend.fault_monitor_backend import FaultStreamWorker
        worker = FaultStreamWorker("192.168.1.42", 50051)
        assert worker._host == "192.168.1.42"

    def test_worker_stores_port(self):
        from ui.backend.fault_monitor_backend import FaultStreamWorker
        worker = FaultStreamWorker("192.168.1.1", 50051)
        assert worker._port == 50051

    def test_worker_not_running_initially(self):
        from ui.backend.fault_monitor_backend import FaultStreamWorker
        worker = FaultStreamWorker("192.168.1.1", 50051)
        assert not worker.isRunning()


# ---------------------------------------------------------------------------
# FaultMonitorBackend - interface (no thread started)
# ---------------------------------------------------------------------------

class TestFaultMonitorBackendInterface:

    def test_backend_has_connect_slot(self):
        from ui.backend.fault_monitor_backend import FaultMonitorBackend
        backend = FaultMonitorBackend()
        assert hasattr(backend, "connectToDevice")
        assert callable(backend.connectToDevice)

    def test_backend_has_disconnect_slot(self):
        from ui.backend.fault_monitor_backend import FaultMonitorBackend
        backend = FaultMonitorBackend()
        assert hasattr(backend, "disconnectFromDevice")
        assert callable(backend.disconnectFromDevice)

    def test_backend_has_event_received_signal(self):
        from ui.backend.fault_monitor_backend import FaultMonitorBackend
        backend = FaultMonitorBackend()
        assert hasattr(backend, "eventReceived")

    def test_backend_has_connected_signal(self):
        from ui.backend.fault_monitor_backend import FaultMonitorBackend
        backend = FaultMonitorBackend()
        assert hasattr(backend, "connected")

    def test_backend_has_disconnected_signal(self):
        from ui.backend.fault_monitor_backend import FaultMonitorBackend
        backend = FaultMonitorBackend()
        assert hasattr(backend, "disconnected")

    def test_backend_has_connection_error_signal(self):
        from ui.backend.fault_monitor_backend import FaultMonitorBackend
        backend = FaultMonitorBackend()
        assert hasattr(backend, "connectionError")

    def test_backend_worker_is_none_initially(self):
        from ui.backend.fault_monitor_backend import FaultMonitorBackend
        backend = FaultMonitorBackend()
        assert backend._worker is None

    def test_disconnect_on_unconnected_backend_does_not_raise(self):
        from ui.backend.fault_monitor_backend import FaultMonitorBackend
        backend = FaultMonitorBackend()
        backend.disconnectFromDevice()