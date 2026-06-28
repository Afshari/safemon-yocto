"""
test_key_management.py - Integration tests for KeyManagementBackend.
SSH is fully mocked - no real device needed.
Requires pytest-qt for signal testing.
"""

import pytest
from pathlib import Path


# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------

@pytest.fixture
def mock_ssh(mocker):
    mock_mgr = mocker.MagicMock()
    mocker.patch(
        "ui.backend.key_management_backend.SSHManager",
        return_value=mock_mgr
    )
    return mock_mgr


@pytest.fixture
def mock_config(mocker, tmp_config_dir):
    import core.config_manager as cm
    cm.ensure_all_configs()
    cm.save_platform("raspberry_pi", {
        "host": "192.168.1.42",
        "user": "root",
        "port": 22,
        "description": "Raspberry Pi 4"
    })


@pytest.fixture
def backend(qtbot):
    from ui.backend.key_management_backend import KeyManagementBackend
    return KeyManagementBackend()


# ---------------------------------------------------------------------------
# generateKeys
# ---------------------------------------------------------------------------

class TestGenerateKeys:

    def test_key_gen_success_emitted(self, qtbot, backend, tmp_path):
        key_dir = tmp_path / "keys"
        key_dir.mkdir()

        with qtbot.waitSignal(backend.keyGenSuccess, timeout=5000):
            backend.generateKeys(str(key_dir))

    def test_key_gen_success_emits_key_path(self, qtbot, backend, tmp_path):
        key_dir = tmp_path / "keys"
        key_dir.mkdir()

        with qtbot.waitSignal(backend.keyGenSuccess, timeout=5000) as blocker:
            backend.generateKeys(str(key_dir))

        key_path, pub_path = blocker.args
        assert key_path.endswith("safemon.key")

    def test_key_gen_success_emits_pub_path(self, qtbot, backend, tmp_path):
        key_dir = tmp_path / "keys"
        key_dir.mkdir()

        with qtbot.waitSignal(backend.keyGenSuccess, timeout=5000) as blocker:
            backend.generateKeys(str(key_dir))

        key_path, pub_path = blocker.args
        assert pub_path.endswith("safemon.pub")

    def test_key_gen_creates_key_file(self, qtbot, backend, tmp_path):
        key_dir = tmp_path / "keys"
        key_dir.mkdir()

        with qtbot.waitSignal(backend.keyGenSuccess, timeout=5000):
            backend.generateKeys(str(key_dir))

        assert (key_dir / "safemon.key").exists()

    def test_key_gen_creates_pub_file(self, qtbot, backend, tmp_path):
        key_dir = tmp_path / "keys"
        key_dir.mkdir()

        with qtbot.waitSignal(backend.keyGenSuccess, timeout=5000):
            backend.generateKeys(str(key_dir))

        assert (key_dir / "safemon.pub").exists()

    def test_key_gen_creates_directory_if_missing(self, qtbot, backend, tmp_path):
        key_dir = tmp_path / "new_dir" / "keys"
        assert not key_dir.exists()

        with qtbot.waitSignal(backend.keyGenSuccess, timeout=5000):
            backend.generateKeys(str(key_dir))

        assert key_dir.exists()

    def test_key_gen_failed_emitted_on_empty_dir(self, qtbot, backend):
        with qtbot.waitSignal(backend.keyGenFailed, timeout=3000) as blocker:
            backend.generateKeys("   ")

        assert blocker.args[0] != ""

    def test_key_gen_failed_not_emitted_on_success(self, qtbot, backend, tmp_path):
        key_dir = tmp_path / "keys"
        key_dir.mkdir()

        failed_calls = []
        backend.keyGenFailed.connect(lambda e: failed_calls.append(e))

        with qtbot.waitSignal(backend.keyGenSuccess, timeout=5000):
            backend.generateKeys(str(key_dir))

        assert len(failed_calls) == 0


# ---------------------------------------------------------------------------
# copyPublicKey
# ---------------------------------------------------------------------------

class TestCopyPublicKey:

    def test_scp_success_emitted(self, qtbot, backend, mock_ssh, mock_config, tmp_path):
        pub_file = tmp_path / "safemon.pub"
        pub_file.write_text('{"public_key_x": [], "public_key_y": []}',
                            encoding="utf-8", newline="\n")

        with qtbot.waitSignal(backend.scpSuccess, timeout=5000) as blocker:
            backend.copyPublicKey(str(pub_file), "raspberry_pi", "root", "")

        assert blocker.args[0] == "/etc/safemon/pki/safemon.pub"

    def test_scp_calls_put_file_with_correct_dest(
        self, qtbot, backend, mock_ssh, mock_config, tmp_path
    ):
        pub_file = tmp_path / "safemon.pub"
        pub_file.write_text('{"public_key_x": [], "public_key_y": []}',
                            encoding="utf-8", newline="\n")

        with qtbot.waitSignal(backend.scpSuccess, timeout=5000):
            backend.copyPublicKey(str(pub_file), "raspberry_pi", "root", "")

        mock_ssh.put_file.assert_called_once_with(
            str(pub_file), "/etc/safemon/pki/safemon.pub"
        )

    def test_scp_creates_remote_directory(
        self, qtbot, backend, mock_ssh, mock_config, tmp_path
    ):
        pub_file = tmp_path / "safemon.pub"
        pub_file.write_text('{"public_key_x": [], "public_key_y": []}',
                            encoding="utf-8", newline="\n")

        with qtbot.waitSignal(backend.scpSuccess, timeout=5000):
            backend.copyPublicKey(str(pub_file), "raspberry_pi", "root", "")

        commands = [
            call.args[0]
            for call in mock_ssh.run_command.call_args_list
        ]
        assert any("mkdir" in cmd for cmd in commands)

    def test_scp_failed_on_missing_pub_file(
        self, qtbot, backend, mock_ssh, mock_config, tmp_path
    ):
        missing = tmp_path / "nonexistent.pub"

        with qtbot.waitSignal(backend.scpFailed, timeout=3000) as blocker:
            backend.copyPublicKey(str(missing), "raspberry_pi", "root", "")

        assert blocker.args[0] != ""

    def test_scp_failed_on_empty_pub_path(
        self, qtbot, backend, mock_ssh, mock_config
    ):
        with qtbot.waitSignal(backend.scpFailed, timeout=3000) as blocker:
            backend.copyPublicKey("", "raspberry_pi", "root", "")

        assert "No public key" in blocker.args[0]

    def test_scp_failed_on_no_host(self, qtbot, tmp_config_dir, tmp_path):
        import core.config_manager as cm
        cm.ensure_all_configs()

        from ui.backend.key_management_backend import KeyManagementBackend
        backend = KeyManagementBackend()

        pub_file = tmp_path / "safemon.pub"
        pub_file.write_text('{"public_key_x": [], "public_key_y": []}',
                            encoding="utf-8", newline="\n")

        with qtbot.waitSignal(backend.scpFailed, timeout=3000) as blocker:
            backend.copyPublicKey(str(pub_file), "raspberry_pi", "root", "")

        assert blocker.args[0] != ""

    def test_scp_failed_on_ssh_error(
        self, qtbot, backend, mock_ssh, mock_config, tmp_path
    ):
        mock_ssh.connect.side_effect = Exception("Connection refused")

        pub_file = tmp_path / "safemon.pub"
        pub_file.write_text('{"public_key_x": [], "public_key_y": []}',
                            encoding="utf-8", newline="\n")

        with qtbot.waitSignal(backend.scpFailed, timeout=5000):
            backend.copyPublicKey(str(pub_file), "raspberry_pi", "root", "")