using System.Numerics;
using System.Security.Cryptography;

namespace SafemonGui.Crypto;

/// secp256k1 curve parameters and shared modular arithmetic helpers.
public static class Secp256k1
{
    public static readonly BigInteger P =
        BigInteger.Parse("00FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F",
            System.Globalization.NumberStyles.HexNumber);

    public static readonly BigInteger A = 0;
    public static readonly BigInteger B = 7;

    public static readonly BigInteger GX =
        BigInteger.Parse("0079BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798",
            System.Globalization.NumberStyles.HexNumber);

    public static readonly BigInteger GY =
        BigInteger.Parse("00483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8",
            System.Globalization.NumberStyles.HexNumber);

    public static readonly BigInteger N =
    BigInteger.Parse("00FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141",
        System.Globalization.NumberStyles.HexNumber);

    public static ECPoint G => new ECPoint(GX, GY);

    /// True mathematical modulo (always non-negative), matching Python's % operator.
    public static BigInteger Mod(BigInteger value, BigInteger modulus)
    {
        var result = value % modulus;
        return result < 0 ? result + modulus : result;
    }

    /// Modular inverse via extended Euclidean algorithm (equivalent to Python's pow(x, -1, m)).
    public static BigInteger ModInverse(BigInteger value, BigInteger modulus)
    {
        BigInteger a = Mod(value, modulus);
        BigInteger m = modulus;
        BigInteger x0 = 0, x1 = 1;

        if (m == 1)
            return 0;

        while (a > 1)
        {
            var q = a / m;
            (a, m) = (m, a % m);
            (x0, x1) = (x1 - q * x0, x0);
        }

        return Mod(x1, modulus);
    }

    /// Cryptographically random scalar in [1, n-1].
    public static BigInteger RandomScalar()
    {
        Span<byte> bytes = stackalloc byte[33];
        BigInteger k;

        do
        {
            RandomNumberGenerator.Fill(bytes[..32]);
            bytes[32] = 0; // extra byte forces BigInteger to interpret this as positive (little-endian)
            k = new BigInteger(bytes);
        } while (k < 1 || k >= N);

        return k;
    }
}