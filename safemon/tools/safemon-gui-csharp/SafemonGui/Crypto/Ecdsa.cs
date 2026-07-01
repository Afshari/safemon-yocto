using System.Numerics;

namespace SafemonGui.Crypto;

/// ECDSA sign/verify over secp256k1, matching safemon_sign.py behavior.
public static class Ecdsa
{
    public static (BigInteger r, BigInteger s) Sign(BigInteger fileHash, BigInteger privateKey)
    {
        var e = Secp256k1.Mod(fileHash, Secp256k1.N);

        while (true)
        {
            var k = Secp256k1.RandomScalar();
            var r = Secp256k1.Mod((k * Secp256k1.G).X, Secp256k1.N);
            if (r == 0)
                continue;

            var s = Secp256k1.Mod(Secp256k1.ModInverse(k, Secp256k1.N) * (e + r * privateKey), Secp256k1.N);
            if (s != 0)
                return (r, s);
        }
    }

    public static bool Verify(BigInteger fileHash, BigInteger r, BigInteger s, ECPoint publicKey)
    {
        if (r < 1 || r >= Secp256k1.N || s < 1 || s >= Secp256k1.N)
            return false;

        var e = Secp256k1.Mod(fileHash, Secp256k1.N);
        var w = Secp256k1.ModInverse(s, Secp256k1.N);
        var u1 = Secp256k1.Mod(e * w, Secp256k1.N);
        var u2 = Secp256k1.Mod(r * w, Secp256k1.N);

        var x = (u1 * Secp256k1.G) + (u2 * publicKey);
        if (x.IsInfinity)
            return false;

        return Secp256k1.Mod(x.X, Secp256k1.N) == r;
    }
}