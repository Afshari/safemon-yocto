"""
config_manager.py - Loads and saves all JSON config files.
Auto-creates config files with defaults if they do not exist.
"""

import json
import os
from pathlib import Path

# Config directory is always relative to this file's project root
CONFIG_DIR = Path(__file__).resolve().parent.parent / "config"

# --- Default config values ---

DEFAULT_APP = {
    "journal_lines": 50,
    "journal_refresh_seconds": 10,
    "key_dir": str(Path.home() / ".safemon")
}

DEFAULT_RASPBERRY_PI = {
    "host": "",
    "user": "root",
    "port": 22,
    "description": "Raspberry Pi 4"
}

DEFAULT_JETSON_ORIN_NANO = {
    "host": "jetson-orin-nano-devkit.local",
    "user": "root",
    "port": 22,
    "description": "Jetson Orin Nano"
}

DEFAULT_QEMU = {
    "host": "localhost",
    "user": "root",
    "port": 2222,
    "description": "QEMU"
}

DEFAULT_DEVICE_FILES = {
    "safemon.conf": "/etc/safemon/safemon.conf",
    "safemon.conf.sig": "/etc/safemon/safemon.conf.sig"
}

# Map of filename -> default content
CONFIG_FILES = {
    "app.json":              DEFAULT_APP,
    "raspberry_pi.json":     DEFAULT_RASPBERRY_PI,
    "jetson_orin_nano.json": DEFAULT_JETSON_ORIN_NANO,
    "qemu.json":             DEFAULT_QEMU,
    "device_files.json":     DEFAULT_DEVICE_FILES,
}


def ensure_config_dir() -> None:
    """Create the config directory if it does not exist."""
    CONFIG_DIR.mkdir(parents=True, exist_ok=True)


def ensure_all_configs() -> None:
    """
    Called on application startup.
    Creates any missing config files with default values.
    Existing files are never overwritten.
    """
    ensure_config_dir()
    for filename, defaults in CONFIG_FILES.items():
        path = CONFIG_DIR / filename
        if not path.exists():
            _write_json(path, defaults)
            print(f"[config] Created default config: {path}")
        else:
            print(f"[config] Found existing config: {path}")


def load(filename: str) -> dict:
    """
    Load a config file by filename (e.g. 'app.json').
    Returns the parsed dict. Raises FileNotFoundError if missing.
    """
    path = CONFIG_DIR / filename
    if not path.exists():
        raise FileNotFoundError(f"Config file not found: {path}")
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def save(filename: str, data: dict) -> None:
    """
    Save a dict to a config file by filename.
    Overwrites the existing file.
    """
    ensure_config_dir()
    path = CONFIG_DIR / filename
    _write_json(path, data)


def load_app() -> dict:
    return load("app.json")


def load_platform(platform: str) -> dict:
    """
    Load platform config by name.
    platform must be one of: 'raspberry_pi', 'jetson_orin_nano', 'qemu'
    """
    return load(f"{platform}.json")


def save_platform(platform: str, data: dict) -> None:
    save(f"{platform}.json", data)

def load_device_files() -> dict:
    return load("device_files.json")

def _write_json(path: Path, data: dict) -> None:
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)