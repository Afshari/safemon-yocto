using System.Runtime.CompilerServices;
using Grpc.Core;
using Grpc.Net.Client;
using Safemon;
using SafemonGui.Models;

namespace SafemonGui.Core;

public class GrpcFaultClient
{
    private readonly string _host;
    private readonly int _port;

    public GrpcFaultClient(string host, int port)
    {
        _host = host;
        _port = port;
    }

    public async IAsyncEnumerable<FaultEventDisplay> StreamFaultsAsync(
        [EnumeratorCancellation] CancellationToken cancellationToken = default)
    {
        // Grpc.Net.Client defaults to HTTPS; safemon-app serves plaintext gRPC,
        // so we opt in to an insecure (h2c) channel explicitly.
        AppContext.SetSwitch("System.Net.Http.SocketsHttpHandler.Http2UnencryptedSupport", true);

        using var channel = GrpcChannel.ForAddress($"http://{_host}:{_port}",
            new GrpcChannelOptions { Credentials = Grpc.Core.ChannelCredentials.Insecure });

        var client = new FaultService.FaultServiceClient(channel);
        using var call = client.StreamFaults(new StreamRequest(), cancellationToken: cancellationToken);

        await foreach (var evt in call.ResponseStream.ReadAllAsync(cancellationToken))
        {
            yield return new FaultEventDisplay
            {
                Status = evt.Status,
                Timestamp = SafeParseTimestamp(evt.Timestamp),
                Source = evt.Source,
                Signature = evt.Signature
            };
        }
    }

    private static DateTimeOffset SafeParseTimestamp(long rawValue)
    {
        try
        {
            // Original spec says "ms unix", but the .proto comment says seconds --
            // guard against whichever unit is actually being sent by the device.
            return rawValue > 253402300799
                ? DateTimeOffset.FromUnixTimeMilliseconds(rawValue)
                : DateTimeOffset.FromUnixTimeSeconds(rawValue);
        }
        catch (ArgumentOutOfRangeException)
        {
            return DateTimeOffset.UnixEpoch;
        }
    }
}