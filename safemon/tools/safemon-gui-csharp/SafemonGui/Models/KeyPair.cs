using System.Numerics;
using SafemonGui.Crypto;

namespace SafemonGui.Models;

public record KeyPair
{
    public required BigInteger PrivateKey { get; init; }
    public required ECPoint PublicKey { get; init; }
}