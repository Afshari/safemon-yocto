#pragma once

#include <gmp.h>
#include <string>
#include <cstdint>

namespace ecdsa {

class BigInt {
public:
    BigInt();
    explicit BigInt(unsigned long value);
    explicit BigInt(const std::string& hex);
    BigInt(const BigInt& other);
    BigInt(BigInt&& other) noexcept;
    ~BigInt();

    BigInt& operator=(const BigInt& other);
    BigInt& operator=(BigInt&& other) noexcept;

    // Arithmetic
    BigInt operator+(const BigInt& rhs) const;
    BigInt operator-(const BigInt& rhs) const;
    BigInt operator*(const BigInt& rhs) const;
    BigInt operator%(const BigInt& modulus) const;

    // Modular arithmetic
    BigInt mod_add(const BigInt& rhs, const BigInt& modulus) const;
    BigInt mod_sub(const BigInt& rhs, const BigInt& modulus) const;
    BigInt mod_mul(const BigInt& rhs, const BigInt& modulus) const;
    BigInt mod_inv(const BigInt& modulus) const;
    BigInt mod_pow(const BigInt& exp, const BigInt& modulus) const;

    // Comparison
    bool operator==(const BigInt& rhs) const;
    bool operator!=(const BigInt& rhs) const;
    bool operator<(const BigInt& rhs)  const;
    bool operator>(const BigInt& rhs)  const;
    bool is_zero() const;

    // Bit operations
    BigInt operator>>(unsigned int bits) const;
    int bit_length() const;

    // Conversion
    std::string to_hex() const;

    // Raw access for EC operations
    const mpz_t& raw() const { return value_; }
    mpz_t& raw()             { return value_; }

private:
    mpz_t value_;
};

} // namespace ecdsa