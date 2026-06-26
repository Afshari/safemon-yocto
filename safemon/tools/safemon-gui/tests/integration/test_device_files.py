"""
test_device_files.py - Integration tests for DeviceFilesBackend.
SSH is fully mocked - no real device needed.
Requires pytest-qt for signal testing.
"""

import os
import pytest
from pathlib import Path


# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------

@pytest.fixture
def mock_ssh(mocker):
    mock_mgr = mocker.MagicMock()
    mocker.patch(
        "ui.backend.device_files_backend.SSHManager",
        return_value=mock_mgr
    )
    return mock_mgr


@pytest.fixture
def mock_config(tmp_config_dir):
    import core.config_manager as cm
    cm.ensure_all_configs()
    cm.save_platform("raspberry_pi", {
        "host": "192.168.1.42",
        "user": "root",
        "port": 22,
        "description": "Raspberry Pi 4"
    })


@pytest.fixture
def backend(mock_config):
    from ui.backend.device_files_backend import DeviceFilesBackend
    b = DeviceFilesBackend()
    b.loadKnownFiles()
    return b


# ---------------------------------------------------------------------------
# loadKnownFiles
# ---------------------------------------------------------------------------

class TestLoadKnownFiles:

    def test_known_files_loaded_emitted(self, qtbot, mock_config):
        from ui.backend.device_files_backend import DeviceFilesBackend
        b = DeviceFilesBackend()
        with qtbot.waitSignal(b.knownFilesLoaded, timeout=3000) as blocker:
            b.loadKnownFiles()
        assert isinstance(blocker.args[0], list)

    def test_known_files_contains_safemon_conf(self, qtbot, mock_config):
        from ui.backend.device_files_backend import DeviceFilesBackend
        b = DeviceFilesBackend()
        with qtbot.waitSignal(b.knownFilesLoaded, timeout=3000) as blocker:
            b.loadKnownFiles()
        assert "safemon.conf" in blocker.args[0]

    def test_known_files_contains_sig_file(self, qtbot, mock_config):
        from ui.backend.device_files_backend import DeviceFilesBackend
        b = DeviceFilesBackend()
        with qtbot.waitSignal(b.knownFilesLoaded, timeout=3000) as blocker:
            b.loadKnownFiles()
        assert any(".sig" in name for name in blocker.args[0])

    def test_device_path_for_known_file(self, qtbot, backend):
        path = backend.devicePathForFile("safemon.conf")
        assert path == "/etc/safemon/safemon.conf"

    def test_device_path_for_unknown_file_is_empty(self, qtbot, backend):
        path = backend.devicePathForFile("nonexistent.file")
        assert path == ""


# ---------------------------------------------------------------------------
# loadFromDevice
# ---------------------------------------------------------------------------

class TestLoadFromDevice:

    def test_file_loaded_emitted(self, qtbot, backend, mock_ssh, tmp_path):
        content = "drm_device=/dev/dri/card0\n"

        def fake_get(remote, local):
            Path(local).write_text(content, encoding="utf-8")

        mock_ssh.get_file.side_effect = fake_get

        with qtbot.waitSignal(backend.fileLoaded, timeout=5000) as blocker:
            backend.loadFromDevice("safemon.conf", "raspberry_pi", "root", "")

        assert blocker.args[0] == "safemon.conf"
        assert blocker.args[1] == content

    def test_load_failed_on_no_host(self, qtbot, tmp_config_dir):
        import core.config_manager as cm
        cm.ensure_all_configs()

        from ui.backend.device_files_backend import DeviceFilesBackend
        b = DeviceFilesBackend()
        b.loadKnownFiles()

        with qtbot.waitSignal(b.loadFailed, timeout=3000) as blocker:
            b.loadFromDevice("safemon.conf", "raspberry_pi", "root", "")

        assert blocker.args[0] != ""

    def test_load_failed_on_unknown_file(self, qtbot, backend):
        with qtbot.waitSignal(backend.loadFailed, timeout=3000) as blocker:
            backend.loadFromDevice("unknown.file", "raspberry_pi", "root", "")

        assert blocker.args[0] != ""

    def test_load_failed_on_ssh_error(self, qtbot, backend, mock_ssh):
        mock_ssh.connect.side_effect = Exception("SSH error")

        with qtbot.waitSignal(backend.loadFailed, timeout=5000):
            backend.loadFromDevice("safemon.conf", "raspberry_pi", "root", "")


# ---------------------------------------------------------------------------
# copyKnownToDevice
# ---------------------------------------------------------------------------

class TestCopyKnownToDevice:

    def test_copy_success_emitted(self, qtbot, backend, mock_ssh, tmp_path):
        local_file = tmp_path / "safemon.conf"
        local_file.write_text("content\n", encoding="utf-8", newline="\n")

        with qtbot.waitSignal(backend.copySuccess, timeout=5000) as blocker:
            backend.copyKnownToDevice(
                "safemon.conf", str(local_file), "raspberry_pi", "root", ""
            )

        assert blocker.args[0] == "/etc/safemon/safemon.conf"

    def test_copy_calls_put_file_with_correct_paths(
        self, qtbot, backend, mock_ssh, tmp_path
    ):
        local_file = tmp_path / "safemon.conf"
        local_file.write_text("content\n", encoding="utf-8", newline="\n")

        with qtbot.waitSignal(backend.copySuccess, timeout=5000):
            backend.copyKnownToDevice(
                "safemon.conf", str(local_file), "raspberry_pi", "root", ""
            )

        mock_ssh.put_file.assert_called_once_with(
            str(local_file), "/etc/safemon/safemon.conf"
        )

    def test_copy_failed_on_missing_local_file(
        self, qtbot, backend, mock_ssh, tmp_path
    ):
        missing = tmp_path / "nonexistent.conf"

        with qtbot.waitSignal(backend.copyFailed, timeout=3000) as blocker:
            backend.copyKnownToDevice(
                "safemon.conf", str(missing), "raspberry_pi", "root", ""
            )

        assert blocker.args[0] != ""

    def test_copy_failed_on_unknown_file_name(
        self, qtbot, backend, mock_ssh, tmp_path
    ):
        local_file = tmp_path / "something.conf"
        local_file.write_text("content\n", encoding="utf-8", newline="\n")

        with qtbot.waitSignal(backend.copyFailed, timeout=3000) as blocker:
            backend.copyKnownToDevice(
                "unknown.conf", str(local_file), "raspberry_pi", "root", ""
            )

        assert blocker.args[0] != ""

    def test_copy_failed_on_ssh_error(
        self, qtbot, backend, mock_ssh, tmp_path
    ):
        mock_ssh.connect.side_effect = Exception("Connection refused")
        local_file = tmp_path / "safemon.conf"
        local_file.write_text("content\n", encoding="utf-8", newline="\n")

        with qtbot.waitSignal(backend.copyFailed, timeout=5000):
            backend.copyKnownToDevice(
                "safemon.conf", str(local_file), "raspberry_pi", "root", ""
            )


# ---------------------------------------------------------------------------
# copyFileToDevice
# ---------------------------------------------------------------------------

class TestCopyFileToDevice:

    def test_copy_success_emitted(self, qtbot, backend, mock_ssh, tmp_path):
        local_file = tmp_path / "test-can.sh"
        local_file.write_text("#!/bin/sh\n", encoding="utf-8", newline="\n")

        with qtbot.waitSignal(backend.copySuccess, timeout=5000):
            backend.copyFileToDevice(
                str(local_file), "/home/root/", "raspberry_pi", "root", ""
            )

    def test_appends_filename_when_dest_ends_with_slash(
        self, qtbot, backend, mock_ssh, tmp_path
    ):
        local_file = tmp_path / "test-can.sh"
        local_file.write_text("#!/bin/sh\n", encoding="utf-8", newline="\n")

        with qtbot.waitSignal(backend.copySuccess, timeout=5000) as blocker:
            backend.copyFileToDevice(
                str(local_file), "/home/root/", "raspberry_pi", "root", ""
            )

        assert blocker.args[0] == "/home/root/test-can.sh"

    def test_appends_filename_when_dest_has_no_extension(
        self, qtbot, backend, mock_ssh, tmp_path
    ):
        local_file = tmp_path / "myfont.ttf"
        local_file.write_text("font data", encoding="utf-8")

        with qtbot.waitSignal(backend.copySuccess, timeout=5000) as blocker:
            backend.copyFileToDevice(
                str(local_file), "/home/root", "raspberry_pi", "root", ""
            )

        assert blocker.args[0] == "/home/root/myfont.ttf"

    def test_keeps_explicit_full_path(
        self, qtbot, backend, mock_ssh, tmp_path
    ):
        local_file = tmp_path / "test-can.sh"
        local_file.write_text("#!/bin/sh\n", encoding="utf-8", newline="\n")

        with qtbot.waitSignal(backend.copySuccess, timeout=5000) as blocker:
            backend.copyFileToDevice(
                str(local_file), "/home/root/test-can.sh",
                "raspberry_pi", "root", ""
            )

        assert blocker.args[0] == "/home/root/test-can.sh"

    def test_copy_failed_on_missing_local_file(
        self, qtbot, backend, mock_ssh, tmp_path
    ):
        missing = tmp_path / "nonexistent.sh"

        with qtbot.waitSignal(backend.copyFailed, timeout=3000) as blocker:
            backend.copyFileToDevice(
                str(missing), "/home/root/", "raspberry_pi", "root", ""
            )

        assert blocker.args[0] != ""

    def test_copy_failed_on_no_host(self, qtbot, tmp_config_dir, tmp_path):
        import core.config_manager as cm
        cm.ensure_all_configs()

        from ui.backend.device_files_backend import DeviceFilesBackend
        b = DeviceFilesBackend()
        b.loadKnownFiles()

        local_file = tmp_path / "test.sh"
        local_file.write_text("#!/bin/sh\n", encoding="utf-8", newline="\n")

        with qtbot.waitSignal(b.copyFailed, timeout=3000) as blocker:
            b.copyFileToDevice(
                str(local_file), "/home/root/", "raspberry_pi", "root", ""
            )

        assert blocker.args[0] != ""