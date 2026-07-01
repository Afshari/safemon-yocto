using System.IO;
using System.Text;
using System.Text.Json;
using SafemonGui.Models;

namespace SafemonGui.Core;

public class ConfigManager
{
    private readonly string _configDir;
    private static readonly JsonSerializerOptions JsonOptions = new() { WriteIndented = true };

    public ConfigManager(string? configDir = null)
    {
        _configDir = configDir ?? Path.Combine(AppContext.BaseDirectory, "config");
        Directory.CreateDirectory(_configDir);
    }

    // ---------------------------------------------------------------
    // Platform configs
    // ---------------------------------------------------------------

    public PlatformConfig LoadPlatformConfig(string platformName)
    {
        var path = PlatformConfigPath(platformName);

        if (!File.Exists(path))
        {
            var defaults = DefaultPlatformConfig(platformName);
            SavePlatformConfig(platformName, defaults);
            return defaults;
        }

        var json = File.ReadAllText(path);
        return JsonSerializer.Deserialize<PlatformConfig>(json)
            ?? throw new InvalidDataException($"Could not parse platform config: {path}");
    }

    public void SavePlatformConfig(string platformName, PlatformConfig config)
    {
        WriteJsonUnixLineEndings(PlatformConfigPath(platformName), config);
    }

    public void ResetPlatformConfig(string platformName)
    {
        SavePlatformConfig(platformName, DefaultPlatformConfig(platformName));
    }

    private string PlatformConfigPath(string platformName) => Path.Combine(_configDir, $"{platformName}.json");

    private static PlatformConfig DefaultPlatformConfig(string platformName) => platformName switch
    {
        "raspberry_pi" => new PlatformConfig { Host = "192.168.1.147", Port = 22, User = "root" },
        "jetson_orin_nano" => new PlatformConfig { Host = "jetson-orin-nano-devkit.local", Port = 22, User = "root" },
        "qemu" => new PlatformConfig { Host = "172.19.7.64", Port = 2222, User = "root" },
        _ => throw new ArgumentException($"Unknown platform: {platformName}")
    };

    // ---------------------------------------------------------------
    // App settings
    // ---------------------------------------------------------------

    public AppSettings LoadAppSettings()
    {
        var path = AppSettingsPath();

        if (!File.Exists(path))
        {
            var defaults = DefaultAppSettings();
            SaveAppSettings(defaults);
            return defaults;
        }

        var json = File.ReadAllText(path);
        return JsonSerializer.Deserialize<AppSettings>(json)
            ?? throw new InvalidDataException($"Could not parse app settings: {path}");
    }

    public void SaveAppSettings(AppSettings settings)
    {
        WriteJsonUnixLineEndings(AppSettingsPath(), settings);
    }

    public void ResetAppSettings()
    {
        SaveAppSettings(DefaultAppSettings());
    }

    private string AppSettingsPath() => Path.Combine(_configDir, "app.json");

    private static AppSettings DefaultAppSettings() => new()
    {
        JournalLines = 50,
        JournalRefreshSeconds = 10,
        KeyDir = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), ".safemon")
    };

    // ---------------------------------------------------------------
    // Device files (known files list)
    // ---------------------------------------------------------------

    public List<DeviceFileEntry> LoadDeviceFiles()
    {
        var path = DeviceFilesPath();

        if (!File.Exists(path))
        {
            var defaults = DefaultDeviceFiles();
            SaveDeviceFiles(defaults);
            return defaults;
        }

        var json = File.ReadAllText(path);
        return JsonSerializer.Deserialize<List<DeviceFileEntry>>(json)
            ?? throw new InvalidDataException($"Could not parse device files: {path}");
    }

    public void SaveDeviceFiles(List<DeviceFileEntry> entries)
    {
        WriteJsonUnixLineEndings(DeviceFilesPath(), entries);
    }

    private string DeviceFilesPath() => Path.Combine(_configDir, "device_files.json");

    private static List<DeviceFileEntry> DefaultDeviceFiles() => new()
    {
        new DeviceFileEntry { FriendlyName = "safemon.conf", DevicePath = "/etc/safemon/safemon.conf" },
        new DeviceFileEntry { FriendlyName = "safemon.conf.sig", DevicePath = "/etc/safemon/safemon.conf.sig" },
    };

    // ---------------------------------------------------------------
    // Shared helpers
    // ---------------------------------------------------------------

    private static void WriteJsonUnixLineEndings<T>(string path, T data)
    {
        var json = JsonSerializer.Serialize(data, JsonOptions);
        json = json.Replace("\r\n", "\n"); // Unix line endings, matching Python version
        File.WriteAllText(path, json, new UTF8Encoding(encoderShouldEmitUTF8Identifier: false));
    }
}