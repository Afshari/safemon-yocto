"""
test_ssh_manager.py - Unit tests for core/ssh_manager.py.
Paramiko is fully mocked - no real SSH connections are made.
"""

import pytest
from unittest.mock import MagicMock, patch, call
import paramiko

from core.ssh_manager import SSHManager


# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------

@pytest.fixture
def mock_paramiko(mocker):
    """
    Mock the entire paramiko.SSHClient so no real connections are made.
    Returns the mock client instance.
    """
    mock_client = MagicMock()
    mock_transport = MagicMock()
    mock_transport.is_active.return_value = True
    mock_client.get_transport.return_value = mock_transport

    mocker.patch("paramiko.SSHClient", return_value=mock_client)
    mocker.patch("paramiko.AutoAddPolicy")

    return mock_client


@pytest.fixture
def connected_manager(mock_paramiko):
    """An SSHManager that has successfully connected."""
    mgr = SSHManager("192.168.1.1", 22, "root", "")
    mgr.connect()
    return mgr, mock_paramiko


# ---------------------------------------------------------------------------
# Constructor
# ---------------------------------------------------------------------------

class TestSSHManagerInit:

    def test_stores_host(self):
        mgr = SSHManager("192.168.1.1", 22, "root")
        assert mgr.host == "192.168.1.1"

    def test_stores_port(self):
        mgr = SSHManager("192.168.1.1", 22, "root")
        assert mgr.port == 22

    def test_stores_username(self):
        mgr = SSHManager("192.168.1.1", 22, "root")
        assert mgr.username == "root"

    def test_empty_password_stored_as_none(self):
        mgr = SSHManager("192.168.1.1", 22, "root", "")
        assert mgr.password is None

    def test_nonempty_password_stored_as_string(self):
        mgr = SSHManager("192.168.1.1", 22, "root", "secret")
        assert mgr.password == "secret"

    def test_default_password_is_none(self):
        mgr = SSHManager("192.168.1.1", 22, "root")
        assert mgr.password is None

    def test_not_connected_initially(self):
        mgr = SSHManager("192.168.1.1", 22, "root")
        assert not mgr.is_connected()


# ---------------------------------------------------------------------------
# connect
# ---------------------------------------------------------------------------

class TestSSHManagerConnect:

    def test_connect_calls_paramiko_connect(self, mock_paramiko):
        mgr = SSHManager("192.168.1.1", 22, "root", "")
        mgr.connect()
        mock_paramiko.connect.assert_called_once()

    def test_connect_passes_correct_host(self, mock_paramiko):
        mgr = SSHManager("192.168.1.42", 22, "root")
        mgr.connect()
        call_kwargs = mock_paramiko.connect.call_args
        assert call_kwargs.kwargs["hostname"] == "192.168.1.42"

    def test_connect_passes_correct_port(self, mock_paramiko):
        mgr = SSHManager("192.168.1.1", 2222, "root")
        mgr.connect()
        call_kwargs = mock_paramiko.connect.call_args
        assert call_kwargs.kwargs["port"] == 2222

    def test_connect_passes_correct_username(self, mock_paramiko):
        mgr = SSHManager("192.168.1.1", 22, "admin")
        mgr.connect()
        call_kwargs = mock_paramiko.connect.call_args
        assert call_kwargs.kwargs["username"] == "admin"

    def test_connect_sets_missing_host_key_policy(self, mock_paramiko, mocker):
        mgr = SSHManager("192.168.1.1", 22, "root")
        mgr.connect()
        mock_paramiko.set_missing_host_key_policy.assert_called_once()

    def test_connect_fallback_on_ssh_exception(self, mocker):
        mock_client = MagicMock()
        mock_transport = MagicMock()
        mock_transport.is_active.return_value = True
        mock_client.get_transport.return_value = mock_transport

        call_count = 0

        def connect_side_effect(**kwargs):
            nonlocal call_count
            call_count += 1
            if call_count == 1:
                raise paramiko.ssh_exception.SSHException("Invalid key")

        mock_client.connect.side_effect = connect_side_effect
        mocker.patch("paramiko.SSHClient", return_value=mock_client)
        mocker.patch("paramiko.AutoAddPolicy")

        mgr = SSHManager("192.168.1.1", 22, "root")
        mgr.connect()

        assert call_count == 2

    def test_is_connected_true_after_connect(self, connected_manager):
        mgr, _ = connected_manager
        assert mgr.is_connected()


# ---------------------------------------------------------------------------
# run_command
# ---------------------------------------------------------------------------

class TestRunCommand:

    def test_run_command_returns_tuple(self, connected_manager):
        mgr, mock_client = connected_manager

        mock_stdout = MagicMock()
        mock_stderr = MagicMock()
        mock_stdout.channel.recv_exit_status.return_value = 0
        mock_stdout.read.return_value = b"output"
        mock_stderr.read.return_value = b""
        mock_client.exec_command.return_value = (MagicMock(), mock_stdout, mock_stderr)

        result = mgr.run_command("echo hello")
        assert isinstance(result, tuple)
        assert len(result) == 3

    def test_run_command_returns_exit_code(self, connected_manager):
        mgr, mock_client = connected_manager

        mock_stdout = MagicMock()
        mock_stderr = MagicMock()
        mock_stdout.channel.recv_exit_status.return_value = 0
        mock_stdout.read.return_value = b"active"
        mock_stderr.read.return_value = b""
        mock_client.exec_command.return_value = (MagicMock(), mock_stdout, mock_stderr)

        exit_code, out, err = mgr.run_command("systemctl is-active safemon-app")
        assert exit_code == 0

    def test_run_command_returns_stdout(self, connected_manager):
        mgr, mock_client = connected_manager

        mock_stdout = MagicMock()
        mock_stderr = MagicMock()
        mock_stdout.channel.recv_exit_status.return_value = 0
        mock_stdout.read.return_value = b"PONG\n"
        mock_stderr.read.return_value = b""
        mock_client.exec_command.return_value = (MagicMock(), mock_stdout, mock_stderr)

        _, out, _ = mgr.run_command("redis-cli ping")
        assert "PONG" in out

    def test_run_command_returns_stderr(self, connected_manager):
        mgr, mock_client = connected_manager

        mock_stdout = MagicMock()
        mock_stderr = MagicMock()
        mock_stdout.channel.recv_exit_status.return_value = 1
        mock_stdout.read.return_value = b""
        mock_stderr.read.return_value = b"command not found\n"
        mock_client.exec_command.return_value = (MagicMock(), mock_stdout, mock_stderr)

        exit_code, out, err = mgr.run_command("nonexistent-command")
        assert exit_code == 1
        assert "command not found" in err

    def test_run_command_raises_if_not_connected(self):
        mgr = SSHManager("192.168.1.1", 22, "root")
        with pytest.raises(RuntimeError, match="Not connected"):
            mgr.run_command("echo hello")


# ---------------------------------------------------------------------------
# put_file / get_file
# ---------------------------------------------------------------------------

class TestFileTransfer:

    def test_put_file_calls_sftp_put(self, connected_manager):
        mgr, mock_client = connected_manager

        mock_sftp = MagicMock()
        mock_client.open_sftp.return_value = mock_sftp

        mgr.put_file("/local/path/file.txt", "/remote/path/file.txt")
        mock_sftp.put.assert_called_once_with(
            "/local/path/file.txt", "/remote/path/file.txt"
        )

    def test_get_file_calls_sftp_get(self, connected_manager):
        mgr, mock_client = connected_manager

        mock_sftp = MagicMock()
        mock_client.open_sftp.return_value = mock_sftp

        mgr.get_file("/remote/path/file.txt", "/local/path/file.txt")
        mock_sftp.get.assert_called_once_with(
            "/remote/path/file.txt", "/local/path/file.txt"
        )

    def test_put_file_closes_sftp_on_success(self, connected_manager):
        mgr, mock_client = connected_manager

        mock_sftp = MagicMock()
        mock_client.open_sftp.return_value = mock_sftp

        mgr.put_file("/local/file.txt", "/remote/file.txt")
        mock_sftp.close.assert_called_once()

    def test_put_file_closes_sftp_on_error(self, connected_manager):
        mgr, mock_client = connected_manager

        mock_sftp = MagicMock()
        mock_sftp.put.side_effect = IOError("transfer failed")
        mock_client.open_sftp.return_value = mock_sftp

        with pytest.raises(IOError):
            mgr.put_file("/local/file.txt", "/remote/file.txt")

        mock_sftp.close.assert_called_once()

    def test_put_file_raises_if_not_connected(self):
        mgr = SSHManager("192.168.1.1", 22, "root")
        with pytest.raises(RuntimeError, match="Not connected"):
            mgr.put_file("/local/file.txt", "/remote/file.txt")

    def test_get_file_raises_if_not_connected(self):
        mgr = SSHManager("192.168.1.1", 22, "root")
        with pytest.raises(RuntimeError, match="Not connected"):
            mgr.get_file("/remote/file.txt", "/local/file.txt")


# ---------------------------------------------------------------------------
# close
# ---------------------------------------------------------------------------

class TestClose:

    def test_close_calls_client_close(self, connected_manager):
        mgr, mock_client = connected_manager
        mgr.close()
        mock_client.close.assert_called_once()

    def test_is_connected_false_after_close(self, connected_manager):
        mgr, mock_client = connected_manager
        mock_client.get_transport.return_value = None
        mgr.close()
        assert not mgr.is_connected()

    def test_close_sets_client_to_none(self, connected_manager):
        mgr, _ = connected_manager
        mgr.close()
        assert mgr._client is None

    def test_close_on_unconnected_manager_does_not_raise(self):
        mgr = SSHManager("192.168.1.1", 22, "root")
        mgr.close()