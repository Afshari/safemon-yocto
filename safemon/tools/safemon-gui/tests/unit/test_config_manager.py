"""
test_config_manager.py - Unit tests for core/config_manager.py.
No network, no device, no Qt required.
All file I/O goes to a temp directory via the tmp_config_dir fixture.
"""

import json
import pytest
from pathlib import Path

import core.config_manager as cm


# ---------------------------------------------------------------------------
# ensure_config_dir
# ---------------------------------------------------------------------------

class TestEnsureConfigDir:

    def test_creates_directory_if_missing(self, tmp_config_dir):
        new_dir = tmp_config_dir / "subdir"
        cm.CONFIG_DIR = new_dir
        assert not new_dir.exists()
        cm.ensure_config_dir()
        assert new_dir.exists()

    def test_does_not_raise_if_already_exists(self, tmp_config_dir):
        cm.CONFIG_DIR = tmp_config_dir
        tmp_config_dir.mkdir(parents=True, exist_ok=True)
        cm.ensure_config_dir()
        assert tmp_config_dir.exists()


# ---------------------------------------------------------------------------
# ensure_all_configs
# ---------------------------------------------------------------------------

class TestEnsureAllConfigs:

    def test_creates_all_default_config_files(self, tmp_config_dir):
        cm.ensure_all_configs()
        for filename in cm.CONFIG_FILES:
            assert (tmp_config_dir / filename).exists(), \
                f"Expected {filename} to be created"

    def test_created_files_contain_valid_json(self, tmp_config_dir):
        cm.ensure_all_configs()
        for filename in cm.CONFIG_FILES:
            path = tmp_config_dir / filename
            data = json.loads(path.read_text(encoding="utf-8"))
            assert isinstance(data, dict)

    def test_does_not_overwrite_existing_files(self, tmp_config_dir):
        custom_data = {"custom_key": "custom_value"}
        path = tmp_config_dir / "app.json"
        path.write_text(json.dumps(custom_data), encoding="utf-8")

        cm.ensure_all_configs()

        loaded = json.loads(path.read_text(encoding="utf-8"))
        assert loaded == custom_data

    def test_default_app_config_has_required_keys(self, tmp_config_dir):
        cm.ensure_all_configs()
        data = json.loads((tmp_config_dir / "app.json").read_text())
        assert "journal_lines" in data
        assert "journal_refresh_seconds" in data
        assert "key_dir" in data

    def test_default_raspberry_pi_config_has_required_keys(self, tmp_config_dir):
        cm.ensure_all_configs()
        data = json.loads((tmp_config_dir / "raspberry_pi.json").read_text())
        assert "host" in data
        assert "user" in data
        assert "port" in data

    def test_default_qemu_port_is_2222(self, tmp_config_dir):
        cm.ensure_all_configs()
        data = json.loads((tmp_config_dir / "qemu.json").read_text())
        assert data["port"] == 2222

    def test_default_jetson_host_is_set(self, tmp_config_dir):
        cm.ensure_all_configs()
        data = json.loads((tmp_config_dir / "jetson_orin_nano.json").read_text())
        assert data["host"] == "jetson-orin-nano-devkit.local"

    def test_default_raspberry_pi_host_is_empty(self, tmp_config_dir):
        cm.ensure_all_configs()
        data = json.loads((tmp_config_dir / "raspberry_pi.json").read_text())
        assert data["host"] == ""


# ---------------------------------------------------------------------------
# load
# ---------------------------------------------------------------------------

class TestLoad:

    def test_load_returns_dict(self, tmp_config_dir):
        cm.ensure_all_configs()
        data = cm.load("app.json")
        assert isinstance(data, dict)

    def test_load_raises_if_file_missing(self, tmp_config_dir):
        with pytest.raises(FileNotFoundError):
            cm.load("nonexistent.json")

    def test_load_returns_correct_content(self, tmp_config_dir):
        expected = {"foo": "bar", "number": 42}
        path = tmp_config_dir / "test.json"
        path.write_text(json.dumps(expected), encoding="utf-8")
        result = cm.load("test.json")
        assert result == expected


# ---------------------------------------------------------------------------
# save
# ---------------------------------------------------------------------------

class TestSave:

    def test_save_writes_file(self, tmp_config_dir):
        data = {"key": "value"}
        cm.save("myconfig.json", data)
        assert (tmp_config_dir / "myconfig.json").exists()

    def test_save_content_is_valid_json(self, tmp_config_dir):
        data = {"answer": 42}
        cm.save("myconfig.json", data)
        loaded = json.loads((tmp_config_dir / "myconfig.json").read_text())
        assert loaded == data

    def test_save_overwrites_existing_file(self, tmp_config_dir):
        path = tmp_config_dir / "myconfig.json"
        path.write_text(json.dumps({"old": "data"}), encoding="utf-8")
        cm.save("myconfig.json", {"new": "data"})
        loaded = json.loads(path.read_text())
        assert loaded == {"new": "data"}

    def test_save_and_load_roundtrip(self, tmp_config_dir):
        data = {"host": "192.168.1.1", "port": 22, "user": "root"}
        cm.save("roundtrip.json", data)
        result = cm.load("roundtrip.json")
        assert result == data


# ---------------------------------------------------------------------------
# load_app / load_platform / save_platform
# ---------------------------------------------------------------------------

class TestHelperFunctions:

    def test_load_app_returns_app_config(self, tmp_config_dir):
        cm.ensure_all_configs()
        data = cm.load_app()
        assert "journal_lines" in data

    def test_load_platform_raspberry_pi(self, tmp_config_dir):
        cm.ensure_all_configs()
        data = cm.load_platform("raspberry_pi")
        assert "host" in data
        assert "port" in data

    def test_load_platform_qemu(self, tmp_config_dir):
        cm.ensure_all_configs()
        data = cm.load_platform("qemu")
        assert data["port"] == 2222

    def test_load_platform_jetson(self, tmp_config_dir):
        cm.ensure_all_configs()
        data = cm.load_platform("jetson_orin_nano")
        assert data["host"] == "jetson-orin-nano-devkit.local"

    def test_load_platform_missing_raises(self, tmp_config_dir):
        with pytest.raises(FileNotFoundError):
            cm.load_platform("nonexistent_platform")

    def test_save_platform_persists_changes(self, tmp_config_dir):
        cm.ensure_all_configs()
        original = cm.load_platform("raspberry_pi")
        original["host"] = "192.168.1.99"
        cm.save_platform("raspberry_pi", original)
        reloaded = cm.load_platform("raspberry_pi")
        assert reloaded["host"] == "192.168.1.99"


# ---------------------------------------------------------------------------
# Line endings
# ---------------------------------------------------------------------------

class TestLineEndings:

    def test_written_files_use_unix_line_endings(self, tmp_config_dir):
        cm.ensure_all_configs()
        for filename in cm.CONFIG_FILES:
            raw = (tmp_config_dir / filename).read_bytes()
            assert b"\r\n" not in raw, \
                f"{filename} contains Windows line endings (CRLF)"