#include <gtest/gtest.h>
#include "ecdsa.h"

using namespace ecdsa;

// A fixed 32-byte "hash" for deterministic tests
static std::vector<uint8_t> fake_hash(uint8_t fill = 0xab) {
    return std::vector<uint8_t>(32, fill);
}

TEST(ECDSA, KeypairPublicKeyOnCurve) {
    auto kp = generate_keypair();
    EXPECT_TRUE(kp.public_key.is_on_curve());
    EXPECT_FALSE(kp.public_key.is_infinity());
}

TEST(ECDSA, SignProducesNonZeroRS) {
    auto kp  = generate_keypair();
    auto sig = sign(fake_hash(), kp.private_key);
    EXPECT_FALSE(sig.r.is_zero());
    EXPECT_FALSE(sig.s.is_zero());
}

TEST(ECDSA, VerifyValidSignature) {
    auto kp   = generate_keypair();
    auto hash = fake_hash();
    auto sig  = sign(hash, kp.private_key);
    EXPECT_TRUE(verify(hash, sig, kp.public_key));
}

TEST(ECDSA, VerifyWrongHash) {
    auto kp    = generate_keypair();
    auto hash1 = fake_hash(0xab);
    auto hash2 = fake_hash(0xcd);
    auto sig   = sign(hash1, kp.private_key);
    EXPECT_FALSE(verify(hash2, sig, kp.public_key));
}

TEST(ECDSA, VerifyWrongKey) {
    auto kp1  = generate_keypair();
    auto kp2  = generate_keypair();
    auto hash = fake_hash();
    auto sig  = sign(hash, kp1.private_key);
    EXPECT_FALSE(verify(hash, sig, kp2.public_key));
}

TEST(ECDSA, VerifyTamperedSignatureR) {
    auto kp   = generate_keypair();
    auto hash = fake_hash();
    auto sig  = sign(hash, kp.private_key);
    sig.r = BigInt(1UL); // tamper
    EXPECT_FALSE(verify(hash, sig, kp.public_key));
}

TEST(ECDSA, VerifyTamperedSignatureS) {
    auto kp   = generate_keypair();
    auto hash = fake_hash();
    auto sig  = sign(hash, kp.private_key);
    sig.s = BigInt(1UL); // tamper
    EXPECT_FALSE(verify(hash, sig, kp.public_key));
}

TEST(ECDSA, VerifyZeroRRejected) {
    auto kp   = generate_keypair();
    auto hash = fake_hash();
    Signature bad_sig{ BigInt(0UL), BigInt(1UL) };
    EXPECT_FALSE(verify(hash, bad_sig, kp.public_key));
}

TEST(ECDSA, VerifyZeroSRejected) {
    auto kp   = generate_keypair();
    auto hash = fake_hash();
    Signature bad_sig{ BigInt(1UL), BigInt(0UL) };
    EXPECT_FALSE(verify(hash, bad_sig, kp.public_key));
}

TEST(ECDSA, MultipleSignaturesAreUnique) {
    // Same key + same hash should produce different signatures (random k)
    auto kp   = generate_keypair();
    auto hash = fake_hash();
    auto sig1 = sign(hash, kp.private_key);
    auto sig2 = sign(hash, kp.private_key);
    // Both must verify
    EXPECT_TRUE(verify(hash, sig1, kp.public_key));
    EXPECT_TRUE(verify(hash, sig2, kp.public_key));
    // But r values should differ (different k each time)
    EXPECT_NE(sig1.r.to_hex(), sig2.r.to_hex());
}

TEST(ECDSA, HashFileMissing) {
    EXPECT_THROW(hash_file("/nonexistent/path.conf"), std::runtime_error);
}