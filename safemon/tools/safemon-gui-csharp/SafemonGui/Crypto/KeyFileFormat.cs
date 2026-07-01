using System.Numerics;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.IO;
using System.Text;

namespace SafemonGui.Crypto;

public record KeyFileData
{
    [JsonPropertyName("private_key")]
    public string[]? PrivateKey { get; init; }

    [JsonPropertyName("public_key_x")]
    public required string[] PublicKeyX { get; init; }

    [JsonPropertyName("public_key_y")]
    public required string[] PublicKeyY { get; init; }
}

public static class KeyFileFormat
{
    private const int LimbBits = 64;
    private const int LimbCount = 4;
    private static readonly BigInteger LimbMask = (BigInteger.One << LimbBits) - 1;

    public static string[] IntToLimbs(BigInteger value)
    {
        var limbs = new string[LimbCount];
        for (var i = 0; i < LimbCount; i++)
        {
            limbs[i] = (value & LimbMask).ToString("x16");
            value >>= LimbBits;
        }
        return limbs;
    }

    public static BigInteger LimbsToInt(string[] limbs)
    {
        BigInteger result = 0;
        for (var i = 0; i < limbs.Length; i++)
            result |= BigInteger.Parse("0" + limbs[i], System.Globalization.NumberStyles.HexNumber) << (i * LimbBits);
        return result;
    }

    public static (BigInteger? privateKey, ECPoint publicKey) LoadKey(string path)
    {
        var json = File.ReadAllText(path);
        var data = JsonSerializer.Deserialize<KeyFileData>(json)
            ?? throw new InvalidDataException($"Could not parse key file: {path}");

        var pubX = LimbsToInt(data.PublicKeyX);
        var pubY = LimbsToInt(data.PublicKeyY);
        var pub = new ECPoint(pubX, pubY);

        BigInteger? priv = data.PrivateKey is not null ? LimbsToInt(data.PrivateKey) : null;

        return (priv, pub);
    }

    public static void SaveKeyPair(string path, BigInteger privateKey, ECPoint publicKey)
    {
        var data = new KeyFileData
        {
            PrivateKey = IntToLimbs(privateKey),
            PublicKeyX = IntToLimbs(publicKey.X),
            PublicKeyY = IntToLimbs(publicKey.Y),
        };

        WriteJsonUnixLineEndings(path, data);
    }

    public static void SavePublicKey(string path, ECPoint publicKey)
    {
        var data = new KeyFileData
        {
            PublicKeyX = IntToLimbs(publicKey.X),
            PublicKeyY = IntToLimbs(publicKey.Y),
        };

        WriteJsonUnixLineEndings(path, data);
    }

    private static void WriteJsonUnixLineEndings(string path, KeyFileData data)
    {
        var dir = Path.GetDirectoryName(Path.GetFullPath(path));
        if (!string.IsNullOrEmpty(dir))
            Directory.CreateDirectory(dir);

        var options = new JsonSerializerOptions { WriteIndented = true };
        var json = JsonSerializer.Serialize(data, options);
        json = json.Replace("\r\n", "\n"); // enforce Unix line endings, matching Python version

        File.WriteAllText(path, json, new UTF8Encoding(encoderShouldEmitUTF8Identifier: false));
    }

    public static void SaveSignature(string path, BigInteger r, BigInteger s)
    {
        var content = r.ToString("x64", System.Globalization.CultureInfo.InvariantCulture) + "\n"
                    + s.ToString("x64", System.Globalization.CultureInfo.InvariantCulture) + "\n";
        File.WriteAllText(path, content, new UTF8Encoding(encoderShouldEmitUTF8Identifier: false));
    }

    public static (BigInteger r, BigInteger s) LoadSignature(string path)
    {
        var lines = File.ReadAllText(path).Trim()
            .Split('\n', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);

        if (lines.Length != 2)
            throw new InvalidDataException($"Invalid signature file: expected 2 lines, got {lines.Length}");

        var r = BigInteger.Parse("0" + lines[0], System.Globalization.NumberStyles.HexNumber);
        var s = BigInteger.Parse("0" + lines[1], System.Globalization.NumberStyles.HexNumber);
        return (r, s);
    }

    public static BigInteger HashFile(string path)
    {
        using var sha256 = SHA256.Create();
        using var stream = File.OpenRead(path);
        var digest = sha256.ComputeHash(stream); // 32 bytes, standard big-endian byte order

        // Python does int.from_bytes(digest, 'big') -- convert to BigInteger's little-endian
        // convention by reversing byte order, then pad with a zero byte to force a positive value.
        var le = new byte[digest.Length + 1];
        Array.Copy(digest, le, digest.Length);
        Array.Reverse(le, 0, digest.Length); // reverse only the 32 digest bytes, leave the padding 0 at the end
        return new BigInteger(le);
    }
}