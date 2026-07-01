namespace SafemonGui.Models;

public record FaultEventDisplay
{
    public required string Status { get; init; }
    public required DateTimeOffset Timestamp { get; init; }
    public required string Source { get; init; }
    public required string Signature { get; init; }

    public string StatusColorCategory => Status switch
    {
        var s when s.StartsWith("OK:") => "Ok",
        var s when s.StartsWith("WARN:") => "Warning",
        var s when s.StartsWith("ERROR:") => "Error",
        _ => "Unknown"
    };
}