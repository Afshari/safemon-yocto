namespace SafemonGui.Models;

public enum ServiceStatus { Unknown, Ok, Warning, Error }

public record ServiceCheckResult
{
    public required string ServiceName { get; init; }
    public ServiceStatus Status { get; set; } = ServiceStatus.Unknown;
    public string Output { get; set; } = string.Empty;
}