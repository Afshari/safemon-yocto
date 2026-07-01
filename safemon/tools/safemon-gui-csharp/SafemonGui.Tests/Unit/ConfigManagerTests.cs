using SafemonGui.Core;
using SafemonGui.Models;
using Xunit;

namespace SafemonGui.Tests.Unit;

public class ConfigManagerTests : IDisposable
{
    private readonly string _tempDir;
    private readonly ConfigManager _sut;

    public ConfigManagerTests()
    {
        _tempDir = Path.Combine(Path.GetTempPath(), "safemon-test-" + Guid.NewGuid());
        _sut = new ConfigManager(_tempDir);
    }

    public void Dispose()
    {
        if (Directory.Exists(_tempDir))
            Directory.Delete(_tempDir, recursive: true);
    }

    [Fact]
    public void LoadPlatformConfig_CreatesDefaultsOnFirstRun()
    {
        var config = _sut.LoadPlatformConfig("raspberry_pi");

        Assert.Equal("192.168.1.147", config.Host);
        Assert.Equal(22, config.Port);
        Assert.True(File.Exists(Path.Combine(_tempDir, "raspberry_pi.json")));
    }

    [Fact]
    public void SavePlatformConfig_PersistsChanges()
    {
        var updated = new PlatformConfig { Host = "10.0.0.5", Port = 2222, User = "admin" };
        _sut.SavePlatformConfig("qemu", updated);

        var reloaded = _sut.LoadPlatformConfig("qemu");

        Assert.Equal("10.0.0.5", reloaded.Host);
        Assert.Equal(2222, reloaded.Port);
        Assert.Equal("admin", reloaded.User);
    }

    [Fact]
    public void SavedFiles_UseUnixLineEndings()
    {
        _sut.LoadPlatformConfig("raspberry_pi");
        var raw = File.ReadAllText(Path.Combine(_tempDir, "raspberry_pi.json"));

        Assert.DoesNotContain("\r\n", raw);
    }

    [Fact]
    public void LoadAppSettings_CreatesDefaultsOnFirstRun()
    {
        var settings = _sut.LoadAppSettings();

        Assert.Equal(50, settings.JournalLines);
        Assert.Equal(10, settings.JournalRefreshSeconds);
    }

    [Fact]
    public void LoadDeviceFiles_CreatesDefaultsOnFirstRun()
    {
        var files = _sut.LoadDeviceFiles();

        Assert.Contains(files, f => f.FriendlyName == "safemon.conf");
        Assert.Contains(files, f => f.DevicePath == "/etc/safemon/safemon.conf.sig");
    }

    [Fact]
    public void ResetPlatformConfig_RestoresDefaults()
    {
        _sut.SavePlatformConfig("raspberry_pi", new PlatformConfig { Host = "changed", Port = 1, User = "x" });
        _sut.ResetPlatformConfig("raspberry_pi");

        var config = _sut.LoadPlatformConfig("raspberry_pi");
        Assert.Equal("192.168.1.147", config.Host);
    }
}