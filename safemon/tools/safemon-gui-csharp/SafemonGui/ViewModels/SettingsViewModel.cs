using System.IO;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using SafemonGui.Core;
using SafemonGui.Models;

namespace SafemonGui.ViewModels;

public partial class PlatformConfigSectionViewModel : ObservableObject
{
    private readonly ConfigManager _configManager;
    private readonly string _platformKey;

    public string DisplayName { get; }

    [ObservableProperty]
    private string host = string.Empty;

    [ObservableProperty]
    private int port;

    [ObservableProperty]
    private string user = string.Empty;

    public PlatformConfigSectionViewModel(ConfigManager configManager, string platformKey, string displayName)
    {
        _configManager = configManager;
        _platformKey = platformKey;
        DisplayName = displayName;
        LoadFromDisk();
    }

    private void LoadFromDisk()
    {
        var config = _configManager.LoadPlatformConfig(_platformKey);
        Host = config.Host;
        Port = config.Port;
        User = config.User;
    }

    [RelayCommand]
    private void Save()
    {
        _configManager.SavePlatformConfig(_platformKey,
            new PlatformConfig { Host = Host, Port = Port, User = User });
    }

    [RelayCommand]
    private void Reset()
    {
        _configManager.ResetPlatformConfig(_platformKey);
        LoadFromDisk();
    }
}

public partial class SettingsViewModel : ObservableObject
{
    private readonly ConfigManager _configManager;

    public PlatformConfigSectionViewModel RaspberryPi { get; }
    public PlatformConfigSectionViewModel JetsonOrinNano { get; }
    public PlatformConfigSectionViewModel Qemu { get; }

    [ObservableProperty]
    private int journalLines;

    [ObservableProperty]
    private int journalRefreshSeconds;

    [ObservableProperty]
    private string keyDir = string.Empty;

    [ObservableProperty]
    private string saveResultLog = "Save results will appear here...";

    public SettingsViewModel(ConfigManager configManager)
    {
        _configManager = configManager;

        RaspberryPi = new PlatformConfigSectionViewModel(configManager, "raspberry_pi", "Raspberry Pi");
        JetsonOrinNano = new PlatformConfigSectionViewModel(configManager, "jetson_orin_nano", "Jetson Orin Nano");
        Qemu = new PlatformConfigSectionViewModel(configManager, "qemu", "QEMU");

        LoadAppSettingsFromDisk();
    }

    private void LoadAppSettingsFromDisk()
    {
        var settings = _configManager.LoadAppSettings();
        JournalLines = settings.JournalLines;
        JournalRefreshSeconds = settings.JournalRefreshSeconds;
        KeyDir = settings.KeyDir;
    }

    [RelayCommand]
    private void BrowseKeyDir()
    {
        var dialog = new Microsoft.Win32.OpenFolderDialog
        {
            InitialDirectory = Directory.Exists(KeyDir) ? KeyDir : Environment.CurrentDirectory
        };

        if (dialog.ShowDialog() == true)
            KeyDir = dialog.FolderName;
    }

    [RelayCommand]
    private void SaveAppSettings()
    {
        _configManager.SaveAppSettings(new AppSettings
        {
            JournalLines = JournalLines,
            JournalRefreshSeconds = JournalRefreshSeconds,
            KeyDir = KeyDir
        });

        AppendLog("[OK] App settings saved.");
    }

    [RelayCommand]
    private void ResetAppSettings()
    {
        _configManager.ResetAppSettings();
        LoadAppSettingsFromDisk();
        AppendLog("[OK] App settings reset to defaults.");
    }

    private void AppendLog(string line)
    {
        SaveResultLog = SaveResultLog == "Save results will appear here..."
            ? line
            : SaveResultLog + Environment.NewLine + line;
    }
}