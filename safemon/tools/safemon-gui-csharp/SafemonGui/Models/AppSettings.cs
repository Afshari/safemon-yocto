namespace SafemonGui.Models;

public record AppSettings
{
    public int JournalLines { get; init; } = 50;
    public int JournalRefreshSeconds { get; init; } = 10;
    public required string KeyDir { get; init; }
}