namespace SafemonGui.Models;

public record PlatformConfig
{
    public required string Host { get; init; }
    public required int Port { get; init; }
    public required string User { get; init; }
}