#include <gtest/gtest.h>
#include "bigint.h"

using namespace ecdsa;

TEST(BigInt, ConstructFromUlong) {
    BigInt a(42UL);
    EXPECT_EQ(a.to_hex(), "2a");
}

TEST(BigInt, ConstructFromHex) {
    BigInt a("ff");
    EXPECT_EQ(a.to_hex(), "ff");
}

TEST(BigInt, ConstructInvalidHex) {
    EXPECT_THROW(BigInt("xyz"), std::invalid_argument);
}

TEST(BigInt, Addition) {
    BigInt a("0a");
    BigInt b("05");
    EXPECT_EQ((a + b).to_hex(), "f");
}

TEST(BigInt, Subtraction) {
    BigInt a("0a");
    BigInt b("03");
    EXPECT_EQ((a - b).to_hex(), "7");
}

TEST(BigInt, Multiplication) {
    BigInt a("06");
    BigInt b("07");
    EXPECT_EQ((a * b).to_hex(), "2a");
}

TEST(BigInt, Modulo) {
    BigInt a("1f");  // 31
    BigInt m("0a");  // 10
    EXPECT_EQ((a % m).to_hex(), "5");
}

TEST(BigInt, ModAdd) {
    BigInt a("09");
    BigInt b("08");
    BigInt m("0a");
    // (9 + 8) mod 10 = 7
    EXPECT_EQ(a.mod_add(b, m).to_hex(), "7");
}

TEST(BigInt, ModSub) {
    BigInt a("02");
    BigInt b("05");
    BigInt m("0a");
    // (2 - 5) mod 10 = 7
    EXPECT_EQ(a.mod_sub(b, m).to_hex(), "7");
}

TEST(BigInt, ModMul) {
    BigInt a("06");
    BigInt b("07");
    BigInt m("1f");  // 31
    // (6 * 7) mod 31 = 42 mod 31 = 11
    EXPECT_EQ(a.mod_mul(b, m).to_hex(), "b");
}

TEST(BigInt, ModInv) {
    BigInt a("03");
    BigInt m("0b");  // 11 (prime)
    // 3 * 4 = 12 ≡ 1 mod 11
    BigInt inv = a.mod_inv(m);
    BigInt one = a.mod_mul(inv, m);
    EXPECT_EQ(one.to_hex(), "1");
}

TEST(BigInt, ModInvNoInverse) {
    BigInt a("04");
    BigInt m("08");  // gcd(4,8) != 1 — no inverse
    EXPECT_THROW(a.mod_inv(m), std::runtime_error);
}

TEST(BigInt, ModPow) {
    BigInt base("02");
    BigInt exp("0a");   // 10
    BigInt mod("3e9");  // 1001
    // 2^10 = 1024, 1024 mod 1001 = 23
    EXPECT_EQ(base.mod_pow(exp, mod).to_hex(), "17");
}

TEST(BigInt, IsZero) {
    BigInt z(0UL);
    BigInt nz(1UL);
    EXPECT_TRUE(z.is_zero());
    EXPECT_FALSE(nz.is_zero());
}

TEST(BigInt, Comparison) {
    BigInt a("0a");
    BigInt b("0b");
    EXPECT_TRUE(a < b);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(a == a);
    EXPECT_TRUE(a != b);
}

TEST(BigInt, RightShift) {
    BigInt a("10");  // 16
    EXPECT_EQ((a >> 1).to_hex(), "8");
    EXPECT_EQ((a >> 4).to_hex(), "1");
}

TEST(BigInt, BitLength) {
    BigInt a("ff");  // 255 = 8 bits
    EXPECT_EQ(a.bit_length(), 8);
    BigInt b("100"); // 256 = 9 bits
    EXPECT_EQ(b.bit_length(), 9);
}

TEST(BigInt, CopySemantics) {
    BigInt a("deadbeef");
    BigInt b = a;
    EXPECT_EQ(a.to_hex(), b.to_hex());
    // Mutating b should not affect a
    b = BigInt("0");
    EXPECT_EQ(a.to_hex(), "deadbeef");
}