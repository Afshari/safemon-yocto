#pragma once

#include "bigint.h"

namespace ecdsa {

// secp256k1 curve parameters (all values in hex)
struct Secp256k1Params {
    // Field prime
    static const char* P;
    // Curve coefficients y^2 = x^3 + ax + b
    // a = 0 for secp256k1
    static const char* B;
    // Generator point
    static const char* GX;
    static const char* GY;
    // Group order
    static const char* N;
};

class ECPoint {
public:
    // Construct point at infinity (identity element)
    ECPoint();

    // Construct from affine coordinates
    ECPoint(BigInt x, BigInt y);

    // Copy / move
    ECPoint(const ECPoint&)            = default;
    ECPoint(ECPoint&&)                 = default;
    ECPoint& operator=(const ECPoint&) = default;
    ECPoint& operator=(ECPoint&&)      = default;

    // Point at infinity check
    bool is_infinity() const { return infinity_; }

    // Accessors
    const BigInt& x() const { return x_; }
    const BigInt& y() const { return y_; }

    // Point arithmetic (all operations use secp256k1 field prime p)
    ECPoint operator+(const ECPoint& rhs) const;  // point addition
    ECPoint double_point() const;                  // point doubling
    ECPoint operator*(const BigInt& scalar) const; // scalar multiplication

    bool operator==(const ECPoint& rhs) const;
    bool operator!=(const ECPoint& rhs) const;

    // Verify this point lies on the curve
    bool is_on_curve() const;

    // Generator point G
    static ECPoint generator();

private:
    BigInt x_;
    BigInt y_;
    bool   infinity_;
};

} // namespace ecdsa