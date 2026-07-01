using System.Numerics;

namespace SafemonGui.Crypto;

/// Affine point on secp256k1. IsInfinity true represents the point at infinity.
public readonly struct ECPoint
{
    public BigInteger X { get; }
    public BigInteger Y { get; }
    public bool IsInfinity { get; }

    private ECPoint(BigInteger x, BigInteger y, bool isInfinity)
    {
        X = x;
        Y = y;
        IsInfinity = isInfinity;
    }

    public ECPoint(BigInteger x, BigInteger y) : this(x, y, false)
    {
    }

    public static ECPoint Infinity => new ECPoint(BigInteger.Zero, BigInteger.Zero, true);

    public static ECPoint operator +(ECPoint a, ECPoint b)
    {
        if (a.IsInfinity)
            return b;
        if (b.IsInfinity)
            return a;

        if (a.X == b.X)
        {
            if (a.Y != b.Y)
                return Infinity;
            return Double(a);
        }

        // lambda = (y2 - y1) / (x2 - x1) mod p
        var lambda = Secp256k1.Mod((b.Y - a.Y) * Secp256k1.ModInverse(b.X - a.X, Secp256k1.P), Secp256k1.P);
        var x3 = Secp256k1.Mod(lambda * lambda - a.X - b.X, Secp256k1.P);
        var y3 = Secp256k1.Mod(lambda * (a.X - x3) - a.Y, Secp256k1.P);
        return new ECPoint(x3, y3);
    }

    private static ECPoint Double(ECPoint p)
    {
        // lambda = 3x^2 / (2y) mod p -- a = 0 so no a term
        var lambda = Secp256k1.Mod(3 * p.X * p.X * Secp256k1.ModInverse(2 * p.Y, Secp256k1.P), Secp256k1.P);
        var x3 = Secp256k1.Mod(lambda * lambda - 2 * p.X, Secp256k1.P);
        var y3 = Secp256k1.Mod(lambda * (p.X - x3) - p.Y, Secp256k1.P);
        return new ECPoint(x3, y3);
    }

    public static ECPoint operator *(BigInteger scalar, ECPoint point) => Multiply(point, scalar);

    public static ECPoint operator *(ECPoint point, BigInteger scalar) => Multiply(point, scalar);

    private static ECPoint Multiply(ECPoint point, BigInteger k)
    {
        // Double-and-add scalar multiplication
        var result = Infinity;
        var addend = point;

        while (k > 0)
        {
            if ((k & 1) == 1)
                result = result + addend;
            addend = Double(addend);
            k >>= 1;
        }

        return result;
    }

    public bool IsOnCurve()
    {
        if (IsInfinity)
            return true;

        var lhs = Secp256k1.Mod(Y * Y - X * X * X - Secp256k1.B, Secp256k1.P);
        return lhs == 0;
    }
}