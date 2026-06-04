#include "ecdsa_verify_file.h"
#include "ecdsa.h"
#include "ec_point.h"
#include "bigint.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

// JSON parsing is minimal -- no external dependency.
// Expects the exact format produced by safemon_sign.py:
// {
//     "public_key_x": ["...", "...", "...", "..."],
//     "public_key_y": ["...", "...", "...", "..."]
// }
// Limbs are little-endian: limb[0] = least significant 64 bits.

namespace ecdsa {

static const int LIMB_BITS  = 64;
static const int LIMB_COUNT = 4;

// Reconstruct a 256-bit BigInt from 4 little-endian hex limb strings.
static BigInt limbs_to_bigint(const std::string limbs[4]) {
    // full = limb[3] << 192 | limb[2] << 128 | limb[1] << 64 | limb[0]
    BigInt result(0UL);
    for (int i = LIMB_COUNT - 1; i >= 0; --i) {
        mpz_mul_2exp(result.raw(), result.raw(), LIMB_BITS);
        BigInt limb(limbs[i]);
        mpz_add(result.raw(), result.raw(), limb.raw());
    }
    return result;
}

// Minimal extraction of a JSON string array by key.
// Returns false if key not found or array does not have exactly LIMB_COUNT elements.
static bool extract_limbs(const std::string& json,
                           const std::string& key,
                           std::string        out[4]) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return false;

    pos = json.find('[', pos);
    if (pos == std::string::npos) return false;
    size_t end = json.find(']', pos);
    if (end == std::string::npos) return false;

    std::string array = json.substr(pos + 1, end - pos - 1);

    // Extract quoted strings from array
    int count = 0;
    size_t p  = 0;
    while (count < LIMB_COUNT) {
        size_t q1 = array.find('"', p);
        if (q1 == std::string::npos) break;
        size_t q2 = array.find('"', q1 + 1);
        if (q2 == std::string::npos) break;
        out[count++] = array.substr(q1 + 1, q2 - q1 - 1);
        p = q2 + 1;
    }
    return count == LIMB_COUNT;
}

static ECPoint load_public_key(const std::string& pub_key_path) {
    std::ifstream f(pub_key_path);
    if (!f.is_open()) {
        throw std::runtime_error("verify_file: cannot open public key: " + pub_key_path);
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string json = ss.str();

    std::string x_limbs[4], y_limbs[4];
    if (!extract_limbs(json, "public_key_x", x_limbs)) {
        throw std::runtime_error("verify_file: missing public_key_x in " + pub_key_path);
    }
    if (!extract_limbs(json, "public_key_y", y_limbs)) {
        throw std::runtime_error("verify_file: missing public_key_y in " + pub_key_path);
    }

    BigInt x = limbs_to_bigint(x_limbs);
    BigInt y = limbs_to_bigint(y_limbs);
    return ECPoint(std::move(x), std::move(y));
}

static Signature load_signature(const std::string& sig_path) {
    std::ifstream f(sig_path);
    if (!f.is_open()) {
        throw std::runtime_error("verify_file: cannot open signature: " + sig_path);
    }
    std::string r_hex, s_hex;
    if (!std::getline(f, r_hex) || !std::getline(f, s_hex)) {
        throw std::runtime_error("verify_file: invalid signature file: " + sig_path);
    }
    // trim trailing whitespace
    auto trim = [](std::string& s) {
        size_t end = s.find_last_not_of(" \t\r\n");
        if (end != std::string::npos) s = s.substr(0, end + 1);
    };
    trim(r_hex);
    trim(s_hex);
    return Signature{ BigInt(r_hex), BigInt(s_hex) };
}

bool verify_file(const std::string& file_path,
                 const std::string& pub_key_path) {
    try {
        ECPoint   pub_key = load_public_key(pub_key_path);
        Signature sig     = load_signature(file_path + ".sig");
        auto      digest  = hash_file(file_path);
        return verify(digest, sig, pub_key);
    } catch (const std::exception& e) {
        std::cerr << "[ecdsa] " << e.what() << "\n";
        return false;
    }
}

} // namespace ecdsa