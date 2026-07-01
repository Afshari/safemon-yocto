namespace SafemonGui.Models;

public record DeviceFileEntry
{
    public required string FriendlyName { get; init; }
    public required string DevicePath { get; init; }
}