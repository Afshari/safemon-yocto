#include "ec_point.h"
#include <stdexcept>

namespace ecdsa {

// secp256k1 curve parameters
const char* Secp256k1Params::P  =
    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F";
const char* Secp256k1Params::B  = "7";
const char* Secp256k1Params::GX =
    "79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798";
const char* Secp256k1Params::GY =
    "483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8";
const char* Secp256k1Params::N  =
    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141";

// Lazily initialised curve prime - shared across all operations
static const BigInt& field_prime() {
    static BigInt p(Secp256k1Params::P);
    return p;
}

// Point at infinity (identity element)
ECPoint::ECPoint() : infinity_(true) {}

ECPoint::ECPoint(BigInt x, BigInt y)
    : x_(std::move(x)), y_(std::move(y)), infinity_(false) {}

// Point addition: P + Q
// Handles identity cases, then applies the standard affine addition formula.
ECPoint ECPoint::operator+(const ECPoint& rhs) const {
    if (infinity_)     return rhs;
    if (rhs.infinity_) return *this;

    const BigInt& p = field_prime();

    // P + (-P) = infinity
    BigInt neg_y = p - y_;
    neg_y = neg_y % p;
    if (x_ == rhs.x_) {
        if (y_ == rhs.y_) {
            return double_point();
        }
        return ECPoint(); // point at infinity
    }

    // lam = (y2 - y1) / (x2 - x1) mod p
    BigInt dy  = rhs.y_.mod_sub(y_, p);
    BigInt dx  = rhs.x_.mod_sub(x_, p);
    BigInt lam = dy.mod_mul(dx.mod_inv(p), p);

    // x3 = lam^2 - x1 - x2 mod p
    BigInt x3 = lam.mod_mul(lam, p).mod_sub(x_, p).mod_sub(rhs.x_, p);

    // y3 = lam(x1 - x3) - y1 mod p
    BigInt y3 = lam.mod_mul(x_.mod_sub(x3, p), p).mod_sub(y_, p);

    return ECPoint(x3, y3);
}

// Point doubling: 2P
// Uses the tangent formula. a = 0 for secp256k1 simplifies the numerator.
ECPoint ECPoint::double_point() const {
    if (infinity_) return *this;

    const BigInt& p = field_prime();

    // lam = (3x^2 + a) / (2y) mod p  - a = 0 so numerator is 3x^2
    BigInt three(3UL);
    BigInt two(2UL);

    BigInt num = three.mod_mul(x_.mod_mul(x_, p), p);
    BigInt den = two.mod_mul(y_, p);
    BigInt lam = num.mod_mul(den.mod_inv(p), p);

    // x3 = lam^2 - 2x mod p
    BigInt x3 = lam.mod_mul(lam, p).mod_sub(x_, p).mod_sub(x_, p);

    // y3 = lam(x - x3) - y mod p
    BigInt y3 = lam.mod_mul(x_.mod_sub(x3, p), p).mod_sub(y_, p);

    return ECPoint(x3, y3);
}

// Scalar multiplication: k * P
// Double-and-add (left-to-right binary method).
ECPoint ECPoint::operator*(const BigInt& scalar) const {
    if (infinity_ || scalar.is_zero()) return ECPoint(); // identity

    ECPoint result;   // starts as point at infinity
    ECPoint addend = *this;

    int bits = scalar.bit_length();
    for (int i = 0; i < bits; ++i) {
        // Test bit i of scalar
        BigInt bit = (scalar >> i) % BigInt(2UL);
        if (bit != BigInt(0UL)) {
            result = result + addend;
        }
        addend = addend.double_point();
    }
    return result;
}

bool ECPoint::operator==(const ECPoint& rhs) const {
    if (infinity_ && rhs.infinity_) return true;
    if (infinity_ != rhs.infinity_) return false;
    return x_ == rhs.x_ && y_ == rhs.y_;
}

bool ECPoint::operator!=(const ECPoint& rhs) const {
    return !(*this == rhs);
}

// Check y^2 = x^3 + 7 mod p
bool ECPoint::is_on_curve() const {
    if (infinity_) return true;
    const BigInt& p = field_prime();
    BigInt lhs = y_.mod_mul(y_, p);
    BigInt rhs = x_.mod_mul(x_, p).mod_mul(x_, p)
                   .mod_add(BigInt(Secp256k1Params::B), p);
    return lhs == rhs;
}

ECPoint ECPoint::generator() {
    return ECPoint(
        BigInt(Secp256k1Params::GX),
        BigInt(Secp256k1Params::GY)
    );
}

} // namespace ecdsa