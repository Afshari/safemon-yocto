#pragma once

#include "bigint.h"
#include "ec_point.h"
#include <string>
#include <vector>
#include <cstdint>

namespace ecdsa {

struct KeyPair {
    BigInt  private_key;   // random scalar d, 1 <= d < n
    ECPoint public_key;    // Q = d * G
};

struct Signature {
    BigInt r;
    BigInt s;
};

// Generate a key pair using OS random source (/dev/urandom)
KeyPair generate_keypair();

// Sign a raw message hash (32 bytes for SHA-256)
// Returns signature (r, s)
Signature sign(const std::vector<uint8_t>& hash, const BigInt& private_key);

// Verify a signature against a message hash and public key
// Returns true if valid
bool verify(const std::vector<uint8_t>& hash,
            const Signature&            sig,
            const ECPoint&              public_key);

// Convenience: hash a file and return 32-byte SHA-256 digest
// Requires linking against libcrypto (OpenSSL) - optional helper
std::vector<uint8_t> hash_file(const std::string& path);

} // namespace ecdsa