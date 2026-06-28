"""
conftest.py - Shared pytest fixtures for safemon-gui tests.
"""

import json
import pytest
from pathlib import Path

import sys
from pathlib import Path

SAFEMON_SIGN_DIR = Path(__file__).resolve().parent.parent.parent / "safemon-sign"
PROTO_DIR = Path(__file__).resolve().parent.parent.parent / "proto"
sys.path.insert(0, str(SAFEMON_SIGN_DIR))
sys.path.insert(0, str(PROTO_DIR))

@pytest.fixture
def tmp_config_dir(tmp_path, monkeypatch):
    """
    Redirect CONFIG_DIR to a temp directory so tests never
    touch the real config/ folder.
    """
    import core.config_manager as cm
    monkeypatch.setattr(cm, "CONFIG_DIR", tmp_path)
    return tmp_path


@pytest.fixture
def tmp_key_dir(tmp_path):
    """Temporary directory for key generation tests."""
    key_dir = tmp_path / ".safemon"
    key_dir.mkdir()
    return key_dir