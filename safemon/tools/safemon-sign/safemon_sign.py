#!/usr/bin/env python3
"""
safemon-sign -- ECDSA signing tool for safemon config and calibration files.

Commands:
    keygen   Generate a secp256k1 key pair
    sign     Sign a file with the private key
    verify   Verify a file signature against the public key

Key files use JSON format with 4 little-endian 64-bit hex limbs.
Signature files use hex text format (r and s on separate lines).

Usage:
    python3 safemon_sign.py keygen  --out ~/.safemon/safemon.key
    python3 safemon_sign.py sign    --key ~/.safemon/safemon.key --file safemon.conf
    python3 safemon_sign.py verify  --pub safemon.pub --file safemon.conf --sig safemon.conf.sig
"""

import argparse
import hashlib
import json
import os
import sys

# ---------------------------------------------------------------------------
# secp256k1 curve parameters
# ---------------------------------------------------------------------------

P  = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F
A  = 0
B  = 7
GX = 0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798
GY = 0x483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8
N  = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141

# ---------------------------------------------------------------------------
# Elliptic curve point arithmetic
# ---------------------------------------------------------------------------

class ECPoint:
    """Affine point on secp256k1. None represents the point at infinity."""

    def __init__(self, x, y):
        self.x = x
        self.y = y

    @staticmethod
    def infinity():
        p = ECPoint.__new__(ECPoint)
        p.x = None
        p.y = None
        return p

    def is_infinity(self):
        return self.x is None

    def __eq__(self, other):
        return self.x == other.x and self.y == other.y

    def __add__(self, other):
        if self.is_infinity():
            return other
        if other.is_infinity():
            return self
        if self.x == other.x:
            if self.y != other.y:
                return ECPoint.infinity()
            return self._double()
        # λ = (y2 - y1) / (x2 - x1) mod p
        lam = (other.y - self.y) * pow(other.x - self.x, -1, P) % P
        x3  = (lam * lam - self.x - other.x) % P
        y3  = (lam * (self.x - x3) - self.y) % P
        return ECPoint(x3, y3)

    def _double(self):
        # λ = 3x² / (2y) mod p  -- a = 0 so no a term
        lam = 3 * self.x * self.x * pow(2 * self.y, -1, P) % P
        x3  = (lam * lam - 2 * self.x) % P
        y3  = (lam * (self.x - x3) - self.y) % P
        return ECPoint(x3, y3)

    def __rmul__(self, scalar):
        return self._mul(scalar)

    def __mul__(self, scalar):
        return self._mul(scalar)

    def _mul(self, k):
        """Double-and-add scalar multiplication."""
        result = ECPoint.infinity()
        addend = ECPoint(self.x, self.y)
        while k:
            if k & 1:
                result = result + addend
            addend = addend._double()
            k >>= 1
        return result

    def is_on_curve(self):
        if self.is_infinity():
            return True
        return (self.y * self.y - self.x * self.x * self.x - B) % P == 0


G = ECPoint(GX, GY)

# ---------------------------------------------------------------------------
# Key encoding -- little-endian 64-bit limbs
# ---------------------------------------------------------------------------

LIMB_BITS  = 64
LIMB_MASK  = (1 << LIMB_BITS) - 1
LIMB_COUNT = 4

def int_to_limbs(value):
    """Convert a 256-bit integer to a list of 4 little-endian 64-bit hex strings."""
    limbs = []
    for _ in range(LIMB_COUNT):
        limbs.append(format(value & LIMB_MASK, '016x'))
        value >>= LIMB_BITS
    return limbs

def limbs_to_int(limbs):
    """Reconstruct a 256-bit integer from 4 little-endian 64-bit hex strings."""
    result = 0
    for i, limb in enumerate(limbs):
        result |= int(limb, 16) << (i * LIMB_BITS)
    return result

def load_key(path):
    """Load a key JSON file. Returns (private_key, public_key) or (None, public_key)."""
    with open(path) as f:
        data = json.load(f)

    pub_x = limbs_to_int(data['public_key_x'])
    pub_y = limbs_to_int(data['public_key_y'])
    pub   = ECPoint(pub_x, pub_y)

    priv = None
    if 'private_key' in data:
        priv = limbs_to_int(data['private_key'])

    return priv, pub

def save_keypair(path, private_key, public_key):
    """Save a full key pair to a JSON file."""
    data = {
        'private_key':  int_to_limbs(private_key),
        'public_key_x': int_to_limbs(public_key.x),
        'public_key_y': int_to_limbs(public_key.y),
    }
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, 'w') as f:
        json.dump(data, f, indent=4)

def save_pubkey(path, public_key):
    """Save only the public key to a JSON file."""
    data = {
        'public_key_x': int_to_limbs(public_key.x),
        'public_key_y': int_to_limbs(public_key.y),
    }
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, 'w') as f:
        json.dump(data, f, indent=4)

# ---------------------------------------------------------------------------
# Signature encoding -- hex text (r and s on separate lines)
# ---------------------------------------------------------------------------

def save_signature(path, r, s):
    with open(path, 'w') as f:
        f.write(format(r, '064x') + '\n')
        f.write(format(s, '064x') + '\n')

def load_signature(path):
    with open(path) as f:
        lines = f.read().strip().splitlines()
    if len(lines) != 2:
        raise ValueError(f"Invalid signature file: expected 2 lines, got {len(lines)}")
    return int(lines[0], 16), int(lines[1], 16)

# ---------------------------------------------------------------------------
# Random scalar generation
# ---------------------------------------------------------------------------

def random_scalar():
    """Generate a cryptographically random scalar in [1, n-1]."""
    while True:
        k = int.from_bytes(os.urandom(32), 'big')
        if 1 <= k < N:
            return k

# ---------------------------------------------------------------------------
# Hash
# ---------------------------------------------------------------------------

def hash_file(path):
    """Return SHA-256 digest of a file as an integer."""
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(4096), b''):
            h.update(chunk)
    return int.from_bytes(h.digest(), 'big')

# ---------------------------------------------------------------------------
# ECDSA sign and verify
# ---------------------------------------------------------------------------

def sign(file_hash, private_key):
    """
    ECDSA sign.
      k = random scalar
      R = k * G,  r = R.x mod n
      s = k^-1 * (e + r*d) mod n
    """
    e = file_hash % N
    while True:
        k = random_scalar()
        R = k * G
        r = R.x % N
        if r == 0:
            continue
        s = pow(k, -1, N) * (e + r * private_key) % N
        if s != 0:
            return r, s

def verify(file_hash, r, s, public_key):
    """
    ECDSA verify.
      w  = s^-1 mod n
      u1 = e*w mod n
      u2 = r*w mod n
      X  = u1*G + u2*Q
      valid iff X.x mod n == r
    """
    if not (1 <= r < N and 1 <= s < N):
        return False
    e  = file_hash % N
    w  = pow(s, -1, N)
    u1 = e * w % N
    u2 = r * w % N
    X  = u1 * G + u2 * public_key
    if X.is_infinity():
        return False
    return X.x % N == r

# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

def cmd_keygen(args):
    private_key = random_scalar()
    public_key  = private_key * G

    if not public_key.is_on_curve():
        print("[ERROR] Generated public key is not on curve -- this should never happen.")
        sys.exit(1)

    save_keypair(args.out, private_key, public_key)

    # Derive public key path: same location, .pub extension
    pub_path = args.out.replace('.key', '.pub')
    if pub_path == args.out:
        pub_path = args.out + '.pub'
    save_pubkey(pub_path, public_key)

    print(f"[OK] Private key written to: {args.out}")
    print(f"[OK] Public key written to:  {pub_path}")
    print(f"[OK] Copy {pub_path} to /etc/safemon/pki/ on the device.")

def cmd_sign(args):
    priv, pub = load_key(args.key)
    if priv is None:
        print("[ERROR] Key file does not contain a private key.")
        sys.exit(1)

    file_hash  = hash_file(args.file)
    r, s       = sign(file_hash, priv)
    sig_path   = args.file + '.sig'
    save_signature(sig_path, r, s)

    print(f"[OK] Signed:     {args.file}")
    print(f"[OK] Signature:  {sig_path}")

def cmd_verify(args):
    _, pub    = load_key(args.pub)
    r, s      = load_signature(args.sig)
    file_hash = hash_file(args.file)

    if verify(file_hash, r, s, pub):
        print(f"[OK] Signature is valid for: {args.file}")
    else:
        print(f"[FAIL] Signature is INVALID for: {args.file}")
        sys.exit(1)

# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description='safemon-sign -- ECDSA file signing tool'
    )
    sub = parser.add_subparsers(dest='command', required=True)

    # keygen
    p_keygen = sub.add_parser('keygen', help='Generate a secp256k1 key pair')
    p_keygen.add_argument('--out', required=True,
                          help='Output path for private key (e.g. ~/.safemon/safemon.key)')

    # sign
    p_sign = sub.add_parser('sign', help='Sign a file')
    p_sign.add_argument('--key',  required=True, help='Path to private key JSON file')
    p_sign.add_argument('--file', required=True, help='File to sign')

    # verify
    p_verify = sub.add_parser('verify', help='Verify a file signature')
    p_verify.add_argument('--pub',  required=True, help='Path to public key JSON file')
    p_verify.add_argument('--file', required=True, help='File to verify')
    p_verify.add_argument('--sig',  required=True, help='Signature file (.sig)')

    args = parser.parse_args()

    if args.command == 'keygen':
        cmd_keygen(args)
    elif args.command == 'sign':
        cmd_sign(args)
    elif args.command == 'verify':
        cmd_verify(args)

if __name__ == '__main__':
    main()