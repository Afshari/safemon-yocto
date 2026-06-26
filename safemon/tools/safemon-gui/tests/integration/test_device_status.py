"""
test_device_status.py - Integration tests for DeviceStatusBackend.
SSH is fully mocked - no real device needed.
Requires pytest-qt for signal testing.
"""

import pytest
from unittest.mock import MagicMock, patch


# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------

@pytest.fixture
def mock_ssh(mocker):
    """Mock SSHManager so no real SSH connections are made."""
    mock_mgr = MagicMock()
    mocker.patch("ui.backend.device_status_backend.SSHManager", return_value=mock_mgr)
    return mock_mgr


@pytest.fixture
def mock_config(mocker, tmp_config_dir):
    """Provide a real config via tmp_config_dir."""
    import core.config_manager as cm
    cm.ensure_all_configs()

    rpi_config = {
        "host": "192.168.1.42",
        "user": "root",
        "port": 22,
        "description": "Raspberry Pi 4"
    }
    cm.save_platform("raspberry_pi", rpi_config)
    return rpi_config


@pytest.fixture
def backend(qtbot, mock_ssh, mock_config):
    """A DeviceStatusBackend instance ready to use."""
    from ui.backend.device_status_backend import DeviceStatusBackend
    return DeviceStatusBackend()


# ---------------------------------------------------------------------------
# checkAll - signal emission
# ---------------------------------------------------------------------------

class TestCheckAllSignals:

    def test_check_started_emitted(self, qtbot, backend, mock_ssh):
        mock_ssh.run_command.return_value = (0, "active\n", "")

        with qtbot.waitSignal(backend.checkStarted, timeout=3000):
            backend.checkAll("raspberry_pi", "root", "")

    def test_check_finished_emitted(self, qtbot, backend, mock_ssh):
        mock_ssh.run_command.return_value = (0, "active\n", "")

        with qtbot.waitSignal(backend.checkFinished, timeout=5000):
            backend.checkAll("raspberry_pi", "root", "")

    def test_status_result_emitted_for_all_services(self, qtbot, backend, mock_ssh):
        mock_ssh.run_command.return_value = (0, "active\n", "")

        results = []
        backend.statusResult.connect(lambda name, ok, out: results.append(name))

        with qtbot.waitSignal(backend.checkFinished, timeout=5000):
            backend.checkAll("raspberry_pi", "root", "")

        assert "safemon-app"     in results
        assert "safemon-display" in results
        assert "redis"           in results
        assert "vcan"            in results

    def test_connection_error_emitted_on_no_host(self, qtbot, tmp_config_dir):
        import core.config_manager as cm
        cm.ensure_all_configs()
        # raspberry_pi host is empty by default - no need to set it

        from ui.backend.device_status_backend import DeviceStatusBackend
        backend = DeviceStatusBackend()

        with qtbot.waitSignal(backend.connectionError, timeout=3000) as blocker:
            backend.checkAll("raspberry_pi", "root", "")

        assert "No host" in blocker.args[0]

    def test_connection_error_emitted_on_ssh_failure(self, qtbot, backend, mock_ssh):
        mock_ssh.connect.side_effect = Exception("Connection refused")

        with qtbot.waitSignal(backend.connectionError, timeout=5000):
            backend.checkAll("raspberry_pi", "root", "")


# ---------------------------------------------------------------------------
# checkAll - service evaluation
# ---------------------------------------------------------------------------

class TestServiceEvaluation:

    def _run_check(self, qtbot, backend, mock_ssh, command_results):
        """Helper: run checkAll and collect statusResult emissions."""
        mock_ssh.run_command.side_effect = lambda cmd, **kw: command_results.get(
            next((k for k in command_results if k in cmd), "default"),
            (0, "", "")
        )

        results = {}
        backend.statusResult.connect(
            lambda name, ok, out: results.update({name: ok})
        )

        with qtbot.waitSignal(backend.checkFinished, timeout=5000):
            backend.checkAll("raspberry_pi", "root", "")

        return results

    def test_systemctl_exit_0_is_ok(self, qtbot, backend, mock_ssh):
        mock_ssh.run_command.return_value = (0, "active\n", "")
        results = {}
        backend.statusResult.connect(
            lambda name, ok, out: results.update({name: ok})
        )
        with qtbot.waitSignal(backend.checkFinished, timeout=5000):
            backend.checkAll("raspberry_pi", "root", "")
        assert results.get("safemon-app") is True

    def test_systemctl_exit_3_is_not_ok(self, qtbot, backend, mock_ssh):
        mock_ssh.run_command.return_value = (3, "inactive\n", "")
        results = {}
        backend.statusResult.connect(
            lambda name, ok, out: results.update({name: ok})
        )
        with qtbot.waitSignal(backend.checkFinished, timeout=5000):
            backend.checkAll("raspberry_pi", "root", "")
        assert results.get("safemon-app") is False

    def test_redis_pong_is_ok(self, qtbot, backend, mock_ssh):
        def side_effect(cmd, **kw):
            if "redis" in cmd:
                return (0, "PONG\n", "")
            return (0, "active\n", "")

        mock_ssh.run_command.side_effect = side_effect
        results = {}
        backend.statusResult.connect(
            lambda name, ok, out: results.update({name: ok})
        )
        with qtbot.waitSignal(backend.checkFinished, timeout=5000):
            backend.checkAll("raspberry_pi", "root", "")
        assert results.get("redis") is True

    def test_redis_no_pong_is_not_ok(self, qtbot, backend, mock_ssh):
        def side_effect(cmd, **kw):
            if "redis" in cmd:
                return (1, "", "Could not connect\n")
            return (0, "active\n", "")

        mock_ssh.run_command.side_effect = side_effect
        results = {}
        backend.statusResult.connect(
            lambda name, ok, out: results.update({name: ok})
        )
        with qtbot.waitSignal(backend.checkFinished, timeout=5000):
            backend.checkAll("raspberry_pi", "root", "")
        assert results.get("redis") is False

    def test_vcan_with_vcan0_in_output_is_ok(self, qtbot, backend, mock_ssh):
        def side_effect(cmd, **kw):
            if "vcan" in cmd:
                return (0, "4: vcan0: <NOARP,UP> mtu 72\nvcan 12288 0\n", "")
            return (0, "active\n", "")

        mock_ssh.run_command.side_effect = side_effect
        results = {}
        backend.statusResult.connect(
            lambda name, ok, out: results.update({name: ok})
        )
        with qtbot.waitSignal(backend.checkFinished, timeout=5000):
            backend.checkAll("raspberry_pi", "root", "")
        assert results.get("vcan") is True

    def test_vcan_without_vcan0_is_not_ok(self, qtbot, backend, mock_ssh):
        def side_effect(cmd, **kw):
            if "vcan" in cmd:
                return (1, "", "Device not found\n")
            return (0, "active\n", "")

        mock_ssh.run_command.side_effect = side_effect
        results = {}
        backend.statusResult.connect(
            lambda name, ok, out: results.update({name: ok})
        )
        with qtbot.waitSignal(backend.checkFinished, timeout=5000):
            backend.checkAll("raspberry_pi", "root", "")
        assert results.get("vcan") is False


# ---------------------------------------------------------------------------
# fetchJournal
# ---------------------------------------------------------------------------

class TestFetchJournal:

    def test_journal_loaded_emitted_with_output(self, qtbot, backend, mock_ssh):
        journal_output = "Jun 20 12:31:04 raspberrypi4 safemon-app[391]: Started\n"
        mock_ssh.run_command.return_value = (0, journal_output, "")

        with qtbot.waitSignal(backend.journalLoaded, timeout=5000) as blocker:
            backend.fetchJournal("safemon-app", "raspberry_pi", "root", "")

        assert "safemon-app" in blocker.args[0] or blocker.args[0] == journal_output

    def test_journal_failed_emitted_on_no_host(self, qtbot, backend):
        with qtbot.waitSignal(backend.journalFailed, timeout=3000) as blocker:
            backend.fetchJournal("safemon-app", "raspberry_pi", "root", "")
        assert blocker.args[0] != ""

    def test_journal_failed_emitted_on_ssh_error(self, qtbot, backend, mock_ssh):
        mock_ssh.connect.side_effect = Exception("SSH error")

        with qtbot.waitSignal(backend.journalFailed, timeout=5000):
            backend.fetchJournal("safemon-app", "raspberry_pi", "root", "")

    def test_journal_command_uses_service_name(self, qtbot, backend, mock_ssh):
        mock_ssh.run_command.return_value = (0, "log output\n", "")
        commands_called = []
        mock_ssh.run_command.side_effect = lambda cmd, **kw: (
            commands_called.append(cmd) or (0, "log output\n", "")
        )

        with qtbot.waitSignal(backend.journalLoaded, timeout=5000):
            backend.fetchJournal("safemon-display", "raspberry_pi", "root", "")

        assert any("safemon-display" in cmd for cmd in commands_called)

    def test_journal_command_uses_journal_lines_from_config(
        self, qtbot, backend, mock_ssh, tmp_config_dir
    ):
        import core.config_manager as cm
        cm.save("app.json", {
            "journal_lines": 25,
            "journal_refresh_seconds": 10,
            "key_dir": "/tmp"
        })

        mock_ssh.run_command.return_value = (0, "log\n", "")
        commands_called = []
        mock_ssh.run_command.side_effect = lambda cmd, **kw: (
            commands_called.append(cmd) or (0, "log\n", "")
        )

        with qtbot.waitSignal(backend.journalLoaded, timeout=5000):
            backend.fetchJournal("safemon-app", "raspberry_pi", "root", "")

        assert any("-n 25" in cmd for cmd in commands_called)