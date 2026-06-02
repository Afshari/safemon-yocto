#include "bigint.h"
#include <stdexcept>

namespace ecdsa {

BigInt::BigInt() {
    mpz_init(value_);
}

BigInt::BigInt(unsigned long value) {
    mpz_init_set_ui(value_, value);
}

BigInt::BigInt(const std::string& hex) {
    mpz_init(value_);
    if (mpz_set_str(value_, hex.c_str(), 16) != 0) {
        mpz_clear(value_);
        throw std::invalid_argument("BigInt: invalid hex string: " + hex);
    }
}

BigInt::BigInt(const BigInt& other) {
    mpz_init_set(value_, other.value_);
}

BigInt::BigInt(BigInt&& other) noexcept {
    mpz_init(value_);
    mpz_swap(value_, other.value_);
}

BigInt::~BigInt() {
    mpz_clear(value_);
}

BigInt& BigInt::operator=(const BigInt& other) {
    if (this != &other) {
        mpz_set(value_, other.value_);
    }
    return *this;
}

BigInt& BigInt::operator=(BigInt&& other) noexcept {
    if (this != &other) {
        mpz_swap(value_, other.value_);
    }
    return *this;
}

BigInt BigInt::operator+(const BigInt& rhs) const {
    BigInt result;
    mpz_add(result.value_, value_, rhs.value_);
    return result;
}

BigInt BigInt::operator-(const BigInt& rhs) const {
    BigInt result;
    mpz_sub(result.value_, value_, rhs.value_);
    return result;
}

BigInt BigInt::operator*(const BigInt& rhs) const {
    BigInt result;
    mpz_mul(result.value_, value_, rhs.value_);
    return result;
}

BigInt BigInt::operator%(const BigInt& modulus) const {
    BigInt result;
    mpz_mod(result.value_, value_, modulus.value_);
    return result;
}

BigInt BigInt::mod_add(const BigInt& rhs, const BigInt& modulus) const {
    BigInt result;
    mpz_add(result.value_, value_, rhs.value_);
    mpz_mod(result.value_, result.value_, modulus.value_);
    return result;
}

BigInt BigInt::mod_sub(const BigInt& rhs, const BigInt& modulus) const {
    BigInt result;
    mpz_sub(result.value_, value_, rhs.value_);
    mpz_mod(result.value_, result.value_, modulus.value_);
    return result;
}

BigInt BigInt::mod_mul(const BigInt& rhs, const BigInt& modulus) const {
    BigInt result;
    mpz_mul(result.value_, value_, rhs.value_);
    mpz_mod(result.value_, result.value_, modulus.value_);
    return result;
}

BigInt BigInt::mod_inv(const BigInt& modulus) const {
    BigInt result;
    if (mpz_invert(result.value_, value_, modulus.value_) == 0) {
        throw std::runtime_error("BigInt::mod_inv: no modular inverse exists");
    }
    return result;
}

BigInt BigInt::mod_pow(const BigInt& exp, const BigInt& modulus) const {
    BigInt result;
    mpz_powm(result.value_, value_, exp.value_, modulus.value_);
    return result;
}

bool BigInt::operator==(const BigInt& rhs) const {
    return mpz_cmp(value_, rhs.value_) == 0;
}

bool BigInt::operator!=(const BigInt& rhs) const {
    return !(*this == rhs);
}

bool BigInt::operator<(const BigInt& rhs) const {
    return mpz_cmp(value_, rhs.value_) < 0;
}

bool BigInt::operator>(const BigInt& rhs) const {
    return mpz_cmp(value_, rhs.value_) > 0;
}

bool BigInt::is_zero() const {
    return mpz_sgn(value_) == 0;
}

BigInt BigInt::operator>>(unsigned int bits) const {
    BigInt result;
    mpz_tdiv_q_2exp(result.value_, value_, bits);
    return result;
}

int BigInt::bit_length() const {
    return static_cast<int>(mpz_sizeinbase(value_, 2));
}

std::string BigInt::to_hex() const {
    char* raw = mpz_get_str(nullptr, 16, value_);
    std::string result(raw);
    void (*freefunc)(void*, size_t);
    mp_get_memory_functions(nullptr, nullptr, &freefunc);
    freefunc(raw, result.size() + 1);
    return result;
}

} // namespace ecdsa