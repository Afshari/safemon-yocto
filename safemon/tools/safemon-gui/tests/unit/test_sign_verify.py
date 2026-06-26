"""
test_sign_verify.py - Unit tests for safemon_sign.py.
No network, no device, no Qt required.
Tests the full sign/verify pipeline including key generation,
file signing, verification, and line ending correctness.
"""

import json
import os
import sys
import pytest
from pathlib import Path

# Add safemon-sign to path so we can import safemon_sign directly
SAFEMON_SIGN_DIR = Path(__file__).resolve().parent.parent.parent.parent / "safemon-sign"
sys.path.insert(0, str(SAFEMON_SIGN_DIR))

import safemon_sign as ss


# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------

@pytest.fixture
def key_dir(tmp_path):
    """Temporary directory for key files."""
    d = tmp_path / "keys"
    d.mkdir()
    return d


@pytest.fixture
def generated_keys(key_dir):
    """Generate a key pair and return (key_path, pub_path)."""
    key_path = key_dir / "test.key"
    pub_path = key_dir / "test.pub"
    private_key = ss.random_scalar()
    public_key  = private_key * ss.G
    ss.save_keypair(str(key_path), private_key, public_key)
    ss.save_pubkey(str(pub_path), public_key)
    return key_path, pub_path


@pytest.fixture
def sample_file(tmp_path):
    """Create a sample text file to sign."""
    f = tmp_path / "sample.conf"
    f.write_text("drm_device=/dev/dri/card0\nresolution=1920x1080\n",
                 encoding="utf-8", newline="\n")
    return f


# ---------------------------------------------------------------------------
# Key generation
# ---------------------------------------------------------------------------

class TestKeyGeneration:

    def test_random_scalar_in_valid_range(self):
        k = ss.random_scalar()
        assert 1 <= k < ss.N

    def test_random_scalar_is_different_each_call(self):
        k1 = ss.random_scalar()
        k2 = ss.random_scalar()
        assert k1 != k2

    def test_public_key_on_curve(self):
        private_key = ss.random_scalar()
        public_key  = private_key * ss.G
        assert public_key.is_on_curve()

    def test_save_keypair_creates_key_file(self, key_dir):
        key_path = key_dir / "safemon.key"
        priv = ss.random_scalar()
        pub  = priv * ss.G
        ss.save_keypair(str(key_path), priv, pub)
        assert key_path.exists()

    def test_save_pubkey_creates_pub_file(self, key_dir):
        pub_path = key_dir / "safemon.pub"
        priv = ss.random_scalar()
        pub  = priv * ss.G
        ss.save_pubkey(str(pub_path), pub)
        assert pub_path.exists()

    def test_key_file_is_valid_json(self, generated_keys):
        key_path, _ = generated_keys
        data = json.loads(key_path.read_text(encoding="utf-8"))
        assert "private_key" in data
        assert "public_key_x" in data
        assert "public_key_y" in data

    def test_pub_file_has_no_private_key(self, generated_keys):
        _, pub_path = generated_keys
        data = json.loads(pub_path.read_text(encoding="utf-8"))
        assert "private_key" not in data
        assert "public_key_x" in data
        assert "public_key_y" in data

    def test_key_file_has_correct_limb_count(self, generated_keys):
        key_path, _ = generated_keys
        data = json.loads(key_path.read_text(encoding="utf-8"))
        assert len(data["private_key"])  == 4
        assert len(data["public_key_x"]) == 4
        assert len(data["public_key_y"]) == 4

    def test_load_key_returns_private_and_public(self, generated_keys):
        key_path, _ = generated_keys
        priv, pub = ss.load_key(str(key_path))
        assert priv is not None
        assert pub is not None

    def test_load_pubkey_only_returns_none_private(self, generated_keys):
        _, pub_path = generated_keys
        priv, pub = ss.load_key(str(pub_path))
        assert priv is None
        assert pub is not None


# ---------------------------------------------------------------------------
# Limb encoding
# ---------------------------------------------------------------------------

class TestLimbEncoding:

    def test_int_to_limbs_roundtrip(self):
        value = ss.N - 1
        limbs = ss.int_to_limbs(value)
        result = ss.limbs_to_int(limbs)
        assert result == value

    def test_limbs_count_is_four(self):
        limbs = ss.int_to_limbs(12345)
        assert len(limbs) == 4

    def test_zero_encodes_to_all_zero_limbs(self):
        limbs = ss.int_to_limbs(0)
        assert all(l == "0000000000000000" for l in limbs)


# ---------------------------------------------------------------------------
# Hash
# ---------------------------------------------------------------------------

class TestHashFile:

    def test_hash_file_returns_integer(self, sample_file):
        h = ss.hash_file(str(sample_file))
        assert isinstance(h, int)

    def test_hash_file_is_deterministic(self, sample_file):
        h1 = ss.hash_file(str(sample_file))
        h2 = ss.hash_file(str(sample_file))
        assert h1 == h2

    def test_different_content_gives_different_hash(self, tmp_path):
        f1 = tmp_path / "a.txt"
        f2 = tmp_path / "b.txt"
        f1.write_text("content A", encoding="utf-8", newline="\n")
        f2.write_text("content B", encoding="utf-8", newline="\n")
        assert ss.hash_file(str(f1)) != ss.hash_file(str(f2))


# ---------------------------------------------------------------------------
# Sign and verify
# ---------------------------------------------------------------------------

class TestSignVerify:

    def test_sign_returns_r_and_s(self, sample_file, generated_keys):
        key_path, _ = generated_keys
        priv, pub = ss.load_key(str(key_path))
        file_hash = ss.hash_file(str(sample_file))
        r, s = ss.sign(file_hash, priv)
        assert isinstance(r, int)
        assert isinstance(s, int)
        assert 1 <= r < ss.N
        assert 1 <= s < ss.N

    def test_verify_valid_signature_returns_true(self, sample_file, generated_keys):
        key_path, _ = generated_keys
        priv, pub = ss.load_key(str(key_path))
        file_hash = ss.hash_file(str(sample_file))
        r, s = ss.sign(file_hash, priv)
        assert ss.verify(file_hash, r, s, pub) is True

    def test_verify_wrong_key_returns_false(self, sample_file, generated_keys):
        key_path, _ = generated_keys
        priv, _ = ss.load_key(str(key_path))
        file_hash = ss.hash_file(str(sample_file))
        r, s = ss.sign(file_hash, priv)

        other_priv = ss.random_scalar()
        other_pub  = other_priv * ss.G
        assert ss.verify(file_hash, r, s, other_pub) is False

    def test_verify_tampered_file_returns_false(self, tmp_path, generated_keys):
        key_path, _ = generated_keys
        priv, pub = ss.load_key(str(key_path))

        original = tmp_path / "original.conf"
        original.write_text("original content\n", encoding="utf-8", newline="\n")

        file_hash = ss.hash_file(str(original))
        r, s = ss.sign(file_hash, priv)

        original.write_text("tampered content\n", encoding="utf-8", newline="\n")
        tampered_hash = ss.hash_file(str(original))

        assert ss.verify(tampered_hash, r, s, pub) is False

    def test_sign_produces_different_signatures_each_time(self, sample_file, generated_keys):
        key_path, _ = generated_keys
        priv, _ = ss.load_key(str(key_path))
        file_hash = ss.hash_file(str(sample_file))
        r1, s1 = ss.sign(file_hash, priv)
        r2, s2 = ss.sign(file_hash, priv)
        assert (r1, s1) != (r2, s2)

    def test_verify_rejects_zero_r(self, sample_file, generated_keys):
        key_path, _ = generated_keys
        priv, pub = ss.load_key(str(key_path))
        file_hash = ss.hash_file(str(sample_file))
        assert ss.verify(file_hash, 0, 1, pub) is False

    def test_verify_rejects_zero_s(self, sample_file, generated_keys):
        key_path, _ = generated_keys
        priv, pub = ss.load_key(str(key_path))
        file_hash = ss.hash_file(str(sample_file))
        assert ss.verify(file_hash, 1, 0, pub) is False


# ---------------------------------------------------------------------------
# Signature file I/O
# ---------------------------------------------------------------------------

class TestSignatureIO:

    def test_save_and_load_signature_roundtrip(self, tmp_path, sample_file, generated_keys):
        key_path, _ = generated_keys
        priv, pub = ss.load_key(str(key_path))
        file_hash = ss.hash_file(str(sample_file))
        r, s = ss.sign(file_hash, priv)

        sig_path = tmp_path / "test.sig"
        ss.save_signature(str(sig_path), r, s)
        r2, s2 = ss.load_signature(str(sig_path))

        assert r == r2
        assert s == s2

    def test_load_signature_raises_on_malformed_file(self, tmp_path):
        bad_sig = tmp_path / "bad.sig"
        bad_sig.write_text("only one line\n", encoding="utf-8")
        with pytest.raises(ValueError):
            ss.load_signature(str(bad_sig))

    def test_signature_file_has_two_lines(self, tmp_path, sample_file, generated_keys):
        key_path, _ = generated_keys
        priv, _ = ss.load_key(str(key_path))
        file_hash = ss.hash_file(str(sample_file))
        r, s = ss.sign(file_hash, priv)
        sig_path = tmp_path / "test.sig"
        ss.save_signature(str(sig_path), r, s)
        lines = sig_path.read_text(encoding="utf-8").strip().splitlines()
        assert len(lines) == 2

    def test_signature_lines_are_hex(self, tmp_path, sample_file, generated_keys):
        key_path, _ = generated_keys
        priv, _ = ss.load_key(str(key_path))
        file_hash = ss.hash_file(str(sample_file))
        r, s = ss.sign(file_hash, priv)
        sig_path = tmp_path / "test.sig"
        ss.save_signature(str(sig_path), r, s)
        lines = sig_path.read_text(encoding="utf-8").strip().splitlines()
        for line in lines:
            int(line, 16)


# ---------------------------------------------------------------------------
# Line endings
# ---------------------------------------------------------------------------

class TestLineEndings:

    def test_key_file_uses_unix_line_endings(self, generated_keys):
        key_path, _ = generated_keys
        raw = key_path.read_bytes()
        assert b"\r\n" not in raw, "Key file contains Windows line endings (CRLF)"

    def test_pub_file_uses_unix_line_endings(self, generated_keys):
        _, pub_path = generated_keys
        raw = pub_path.read_bytes()
        assert b"\r\n" not in raw, "Pub file contains Windows line endings (CRLF)"

    def test_sig_file_uses_unix_line_endings(self, tmp_path, sample_file, generated_keys):
        key_path, _ = generated_keys
        priv, _ = ss.load_key(str(key_path))
        file_hash = ss.hash_file(str(sample_file))
        r, s = ss.sign(file_hash, priv)
        sig_path = tmp_path / "test.sig"
        ss.save_signature(str(sig_path), r, s)
        raw = sig_path.read_bytes()
        assert b"\r\n" not in raw, "Sig file contains Windows line endings (CRLF)"


# ---------------------------------------------------------------------------
# Full pipeline
# ---------------------------------------------------------------------------

class TestFullPipeline:

    def test_generate_sign_verify_full_pipeline(self, tmp_path):
        key_path = tmp_path / "safemon.key"
        pub_path = tmp_path / "safemon.pub"
        conf_file = tmp_path / "safemon.conf"
        sig_path  = tmp_path / "safemon.conf.sig"

        conf_file.write_text(
            "drm_device=/dev/dri/card0\nresolution=1920x1080\n",
            encoding="utf-8", newline="\n"
        )

        priv = ss.random_scalar()
        pub  = priv * ss.G
        ss.save_keypair(str(key_path), priv, pub)
        ss.save_pubkey(str(pub_path), pub)

        loaded_priv, _ = ss.load_key(str(key_path))
        file_hash = ss.hash_file(str(conf_file))
        r, s = ss.sign(file_hash, loaded_priv)
        ss.save_signature(str(sig_path), r, s)

        _, loaded_pub = ss.load_key(str(pub_path))
        r2, s2 = ss.load_signature(str(sig_path))
        verify_hash = ss.hash_file(str(conf_file))

        assert ss.verify(verify_hash, r2, s2, loaded_pub) is True