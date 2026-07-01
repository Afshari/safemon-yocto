using System.Numerics;
using SafemonGui.Crypto;
using Xunit;

namespace SafemonGui.Tests.Unit;

public class EcdsaTests
{
    [Fact]
    public void GeneratorPoint_IsOnCurve()
    {
        Assert.True(Secp256k1.G.IsOnCurve());
    }

    [Fact]
    public void KeyGen_ProducesPointOnCurve()
    {
        var priv = Secp256k1.RandomScalar();
        var pub = priv * Secp256k1.G;

        Assert.True(pub.IsOnCurve());
    }

    [Fact]
    public void SignThenVerify_RoundTrips()
    {
        var priv = Secp256k1.RandomScalar();
        var pub = priv * Secp256k1.G;
        var hash = Secp256k1.RandomScalar(); // stand-in for a file hash

        var (r, s) = Ecdsa.Sign(hash, priv);

        Assert.True(Ecdsa.Verify(hash, r, s, pub));
    }

    [Fact]
    public void Verify_FailsWithWrongKey()
    {
        var priv1 = Secp256k1.RandomScalar();
        var priv2 = Secp256k1.RandomScalar();
        var pub2 = priv2 * Secp256k1.G;
        var hash = Secp256k1.RandomScalar();

        var (r, s) = Ecdsa.Sign(hash, priv1);

        Assert.False(Ecdsa.Verify(hash, r, s, pub2));
    }

    [Fact(Skip = "Debug trace - noisy console output, re-enable when investigating sign/verify issues")]
    public void Debug_PrintSignVerifyTrace()
    {
        var priv = Secp256k1.RandomScalar();
        var pub = priv * Secp256k1.G;
        var hash = Secp256k1.RandomScalar();

        Console.WriteLine($"priv = {priv}");
        Console.WriteLine($"pub.X = {pub.X}");
        Console.WriteLine($"pub.Y = {pub.Y}");
        Console.WriteLine($"pub.IsOnCurve() = {pub.IsOnCurve()}");
        Console.WriteLine($"hash = {hash}");

        var (r, s) = Ecdsa.Sign(hash, priv);
        Console.WriteLine($"r = {r}");
        Console.WriteLine($"s = {s}");

        var e = Secp256k1.Mod(hash, Secp256k1.N);
        var w = Secp256k1.ModInverse(s, Secp256k1.N);
        var u1 = Secp256k1.Mod(e * w, Secp256k1.N);
        var u2 = Secp256k1.Mod(r * w, Secp256k1.N);
        Console.WriteLine($"e = {e}");
        Console.WriteLine($"w = {w}");
        Console.WriteLine($"u1 = {u1}");
        Console.WriteLine($"u2 = {u2}");

        var x = (u1 * Secp256k1.G) + (u2 * pub);
        Console.WriteLine($"x.IsInfinity = {x.IsInfinity}");
        Console.WriteLine($"x.X = {x.X}");
        Console.WriteLine($"x.X mod N = {Secp256k1.Mod(x.X, Secp256k1.N)}");

        var verified = Ecdsa.Verify(hash, r, s, pub);
        Console.WriteLine($"verified = {verified}");
    }

    [Fact]
    public void OrderTimesGenerator_IsInfinity()
    {
        var result = Secp256k1.N * Secp256k1.G;
        Assert.True(result.IsInfinity);
    }

    [Fact]
    public void ScalarOne_TimesGenerator_EqualsGenerator()
    {
        var result = BigInteger.One * Secp256k1.G;
        Assert.Equal(Secp256k1.G.X, result.X);
        Assert.Equal(Secp256k1.G.Y, result.Y);
    }
}