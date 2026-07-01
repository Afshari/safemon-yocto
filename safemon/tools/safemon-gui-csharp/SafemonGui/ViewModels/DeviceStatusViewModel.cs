using System.Timers;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using SafemonGui.Core;
using SafemonGui.Models;
using System.Collections.ObjectModel;

namespace SafemonGui.ViewModels;

public partial class DeviceStatusViewModel : ObservableObject, IDisposable
{
    private readonly ConfigManager _configManager;
    private readonly Func<string, int, string, string, SshManager> _sshManagerFactory;
    private System.Timers.Timer? _autoRefreshTimer;

    public string Target { get; set; } = "raspberry_pi";
    public string Username { get; set; } = "root";
    public string Password { get; set; } = string.Empty;

    public ObservableCollection<ServiceCheckResult> ServiceChecks { get; } = new()
    {
        new ServiceCheckResult { ServiceName = "safemon-app" },
        new ServiceCheckResult { ServiceName = "safemon-display" },
        new ServiceCheckResult { ServiceName = "redis" },
        new ServiceCheckResult { ServiceName = "vcan" },
    };

    [ObservableProperty]
    private string checkOutputLog = "Click 'Check All' to run checks...";

    [ObservableProperty]
    private string[] journalServices = { "safemon-app", "safemon-display" };

    [ObservableProperty]
    private string selectedJournalService = "safemon-app";

    [ObservableProperty]
    private string journalOutput = "Select a service and click 'Fetch Now' or 'Start Auto-Refresh'...";

    [ObservableProperty]
    private bool isAutoRefreshing;

    [ObservableProperty]
    private bool isBusy;

    public DeviceStatusViewModel(ConfigManager configManager,
        Func<string, int, string, string, SshManager> sshManagerFactory)
    {
        _configManager = configManager;
        _sshManagerFactory = sshManagerFactory;
    }

    private SshManager CreateSshManager()
    {
        var platformConfig = _configManager.LoadPlatformConfig(Target);
        return _sshManagerFactory(platformConfig.Host, platformConfig.Port, Username, Password);
    }

    [RelayCommand]
    private async Task CheckAllAsync()
    {
        IsBusy = true;
        CheckOutputLog = string.Empty;

        try
        {
            var ssh = CreateSshManager();

            await RunCheck(ssh, "safemon-app", "systemctl status safemon-app",
                result => result.ExitCode == 0 ? ServiceStatus.Ok : ServiceStatus.Error);

            await RunCheck(ssh, "safemon-display", "systemctl status safemon-display",
                result => result.ExitCode == 0 ? ServiceStatus.Ok : ServiceStatus.Error);

            await RunCheck(ssh, "redis", "redis-cli ping",
                result => result.Output.Contains("PONG") ? ServiceStatus.Ok : ServiceStatus.Error);

            await RunCheck(ssh, "vcan", "ip link show vcan0 ; lsmod | grep vcan",
                result => result.Output.Contains("vcan0") ? ServiceStatus.Ok : ServiceStatus.Error);
        }
        catch (Exception ex)
        {
            AppendCheckLog($"[ERROR] {ex.Message}");
        }
        finally
        {
            IsBusy = false;
        }
    }

    private async Task RunCheck(SshManager ssh, string serviceName, string command,
        Func<CommandResult, ServiceStatus> classify)
    {
        var check = ServiceChecks.First(c => c.ServiceName == serviceName);

        try
        {
            var result = await ssh.ExecuteCommandAsync(command);
            check.Status = classify(result);
            check.Output = result.Output;

            AppendCheckLog($"--- {serviceName} ---");
            AppendCheckLog(result.Output.TrimEnd());
            if (!string.IsNullOrWhiteSpace(result.Error))
                AppendCheckLog(result.Error.TrimEnd());
        }
        catch (Exception ex)
        {
            check.Status = ServiceStatus.Error;
            AppendCheckLog($"--- {serviceName} ---");
            AppendCheckLog($"[ERROR] {ex.Message}");
        }

        OnPropertyChanged(nameof(ServiceChecks));
    }

    private void AppendCheckLog(string line)
    {
        CheckOutputLog = string.IsNullOrEmpty(CheckOutputLog) ? line : CheckOutputLog + Environment.NewLine + line;
    }

    [RelayCommand]
    private async Task FetchJournalNowAsync()
    {
        await FetchJournalAsync();
    }

    private async Task FetchJournalAsync()
    {
        try
        {
            var appSettings = _configManager.LoadAppSettings();
            var ssh = CreateSshManager();
            var command = $"journalctl -u {SelectedJournalService} -n {appSettings.JournalLines} --no-pager";

            var result = await ssh.ExecuteCommandAsync(command);
            JournalOutput = result.Succeeded ? result.Output : $"[ERROR] {result.Error}";
        }
        catch (Exception ex)
        {
            JournalOutput = $"[ERROR] {ex.Message}";
        }
    }

    [RelayCommand]
    private void ToggleAutoRefresh()
    {
        if (IsAutoRefreshing)
        {
            StopAutoRefresh();
        }
        else
        {
            StartAutoRefresh();
        }
    }

    private void StartAutoRefresh()
    {
        var appSettings = _configManager.LoadAppSettings();
        var intervalMs = Math.Max(1, appSettings.JournalRefreshSeconds) * 1000;

        _autoRefreshTimer = new System.Timers.Timer(intervalMs) { AutoReset = true };
        _autoRefreshTimer.Elapsed += async (_, _) => await FetchJournalAsync();
        _autoRefreshTimer.Start();

        IsAutoRefreshing = true;

        // Fire an immediate fetch so the user doesn't wait a full interval to see anything.
        _ = FetchJournalAsync();
    }

    private void StopAutoRefresh()
    {
        _autoRefreshTimer?.Stop();
        _autoRefreshTimer?.Dispose();
        _autoRefreshTimer = null;

        IsAutoRefreshing = false;
    }

    public void Dispose()
    {
        StopAutoRefresh();
    }
}