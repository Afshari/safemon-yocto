#include <gtest/gtest.h>
#include "ec_point.h"

using namespace ecdsa;

TEST(ECPoint, GeneratorIsOnCurve) {
    ECPoint G = ECPoint::generator();
    EXPECT_TRUE(G.is_on_curve());
}

TEST(ECPoint, PointAtInfinity) {
    ECPoint inf;
    EXPECT_TRUE(inf.is_infinity());
}

TEST(ECPoint, AddIdentity) {
    ECPoint G   = ECPoint::generator();
    ECPoint inf;
    EXPECT_EQ(G + inf, G);
    EXPECT_EQ(inf + G, G);
}

TEST(ECPoint, DoubleGenerator) {
    ECPoint G  = ECPoint::generator();
    ECPoint G2 = G.double_point();
    EXPECT_FALSE(G2.is_infinity());
    EXPECT_TRUE(G2.is_on_curve());
    EXPECT_NE(G2, G);
}

TEST(ECPoint, AddVsDouble) {
    // G + G should equal 2G
    ECPoint G  = ECPoint::generator();
    ECPoint by_add    = G + G;
    ECPoint by_double = G.double_point();
    EXPECT_EQ(by_add, by_double);
}

TEST(ECPoint, ScalarMulByOne) {
    ECPoint G    = ECPoint::generator();
    ECPoint oneG = G * BigInt(1UL);
    EXPECT_EQ(oneG, G);
}

TEST(ECPoint, ScalarMulByTwo) {
    ECPoint G    = ECPoint::generator();
    ECPoint twoG = G * BigInt(2UL);
    EXPECT_EQ(twoG, G.double_point());
}

TEST(ECPoint, ScalarMulByZero) {
    ECPoint G      = ECPoint::generator();
    ECPoint result = G * BigInt(0UL);
    EXPECT_TRUE(result.is_infinity());
}

TEST(ECPoint, ScalarMulByOrder) {
    // n * G = point at infinity
    ECPoint G = ECPoint::generator();
    BigInt  n(Secp256k1Params::N);
    ECPoint result = G * n;
    EXPECT_TRUE(result.is_infinity());
}

TEST(ECPoint, ScalarMulDistributive) {
    // (a + b) * G == a*G + b*G
    ECPoint G  = ECPoint::generator();
    BigInt  a("123456789abcdef");
    BigInt  b("fedcba987654321");
    BigInt  n(Secp256k1Params::N);
    BigInt  ab = a.mod_add(b, n);

    ECPoint lhs = G * ab;
    ECPoint rhs = (G * a) + (G * b);
    EXPECT_EQ(lhs, rhs);
}

TEST(ECPoint, ResultIsOnCurve) {
    ECPoint G  = ECPoint::generator();
    ECPoint R  = G * BigInt("c0ffee1234567890abcdef");
    EXPECT_TRUE(R.is_on_curve());
}