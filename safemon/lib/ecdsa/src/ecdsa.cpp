#include "ecdsa.h"
#include <openssl/evp.h>
#include <fstream>
#include <stdexcept>
#include <array>

namespace ecdsa {

static const BigInt& curve_order() {
    static BigInt n(Secp256k1Params::N);
    return n;
}

// Generate a cryptographically random BigInt in [1, n-1]
// Reads 32 bytes from /dev/urandom and reduces mod n.
static BigInt random_scalar() {
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    if (!urandom) {
        throw std::runtime_error("random_scalar: cannot open /dev/urandom");
    }

    std::array<unsigned char, 32> buf{};
    urandom.read(reinterpret_cast<char*>(buf.data()), 32);
    if (!urandom) {
        throw std::runtime_error("random_scalar: read from /dev/urandom failed");
    }

    // Build hex string from raw bytes
    std::string hex;
    hex.reserve(64);
    static const char* digits = "0123456789abcdef";
    for (unsigned char b : buf) {
        hex += digits[b >> 4];
        hex += digits[b & 0x0f];
    }

    BigInt k(hex);
    const BigInt& n = curve_order();

    // Reduce mod n, ensure non-zero
    k = k % n;
    if (k.is_zero()) {
        k = BigInt(1UL); // astronomically unlikely — safe fallback
    }
    return k;
}

// Convert 32-byte hash to BigInt, then reduce mod n
static BigInt hash_to_scalar(const std::vector<uint8_t>& hash) {
    if (hash.size() != 32) {
        throw std::invalid_argument("hash_to_scalar: expected 32-byte SHA-256 hash");
    }
    std::string hex;
    hex.reserve(64);
    static const char* digits = "0123456789abcdef";
    for (uint8_t b : hash) {
        hex += digits[b >> 4];
        hex += digits[b & 0x0f];
    }
    return BigInt(hex) % curve_order();
}

KeyPair generate_keypair() {
    BigInt d = random_scalar();
    ECPoint Q = ECPoint::generator() * d;
    return KeyPair{ std::move(d), std::move(Q) };
}

// Sign: given hash e and private key d
//   k = random scalar
//   R = k * G,  r = R.x mod n
//   s = k^-1 * (e + r*d) mod n
Signature sign(const std::vector<uint8_t>& hash, const BigInt& private_key) {
    const BigInt& n = curve_order();
    BigInt e = hash_to_scalar(hash);

    BigInt r, s;
    do {
        BigInt k = random_scalar();
        ECPoint R = ECPoint::generator() * k;
        r = R.x() % n;
        if (r.is_zero()) continue;

        // s = k^-1 * (e + r*d) mod n
        BigInt rd  = r.mod_mul(private_key, n);
        BigInt erd = e.mod_add(rd, n);
        BigInt k_inv = k.mod_inv(n);
        s = k_inv.mod_mul(erd, n);
    } while (s.is_zero());

    return Signature{ r, s };
}

// Verify: given hash e, signature (r, s), public key Q
//   w  = s^-1 mod n
//   u1 = e*w mod n
//   u2 = r*w mod n
//   X  = u1*G + u2*Q
//   valid iff X.x mod n == r
bool verify(const std::vector<uint8_t>& hash,
            const Signature&            sig,
            const ECPoint&              public_key) {
    const BigInt& n = curve_order();

    // Basic range checks
    if (sig.r.is_zero() || sig.s.is_zero()) return false;
    if (!(sig.r < n) || !(sig.s < n))       return false;

    BigInt e = hash_to_scalar(hash);

    BigInt w  = sig.s.mod_inv(n);
    BigInt u1 = e.mod_mul(w, n);
    BigInt u2 = sig.r.mod_mul(w, n);

    ECPoint G  = ECPoint::generator();
    ECPoint X  = (G * u1) + (public_key * u2);

    if (X.is_infinity()) return false;

    BigInt x_mod_n = X.x() % n;
    return x_mod_n == sig.r;
}

std::vector<uint8_t> hash_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("hash_file: cannot open " + path);
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("hash_file: EVP_MD_CTX_new failed");
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("hash_file: EVP_DigestInit_ex failed");
    }

    std::array<char, 4096> buf{};
    while (file.read(buf.data(), buf.size()) || file.gcount() > 0) {
        if (EVP_DigestUpdate(ctx, buf.data(), static_cast<size_t>(file.gcount())) != 1) {
            EVP_MD_CTX_free(ctx);
            throw std::runtime_error("hash_file: EVP_DigestUpdate failed");
        }
    }

    std::vector<uint8_t> digest(32);
    unsigned int digest_len = 0;
    if (EVP_DigestFinal_ex(ctx, digest.data(), &digest_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("hash_file: EVP_DigestFinal_ex failed");
    }

    EVP_MD_CTX_free(ctx);
    return digest;
}
} // namespace ecdsa