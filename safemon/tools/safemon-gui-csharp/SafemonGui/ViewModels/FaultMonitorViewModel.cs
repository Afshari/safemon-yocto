using System.Collections.ObjectModel;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using SafemonGui.Core;
using SafemonGui.Models;

namespace SafemonGui.ViewModels;

public partial class FaultMonitorViewModel : ObservableObject, IDisposable
{
    private readonly ConfigManager _configManager;
    private CancellationTokenSource? _streamCts;
    private Task? _streamTask;

    public string Target { get; set; } = "raspberry_pi";

    public ObservableCollection<FaultEventDisplay> Events { get; } = new();

    [ObservableProperty]
    private int grpcPort = 50051;

    [ObservableProperty]
    private bool isConnected;

    [ObservableProperty]
    private string connectionLog = "Connection log...";

    public int EventCount => Events.Count;

    [RelayCommand]
    private async Task ConnectAsync()
    {
        if (IsConnected)
            return;

        var platformConfig = _configManager.LoadPlatformConfig(Target);
        var client = new GrpcFaultClient(platformConfig.Host, GrpcPort);

        _streamCts = new CancellationTokenSource();
        IsConnected = true;
        AppendLog($"[OK] Connecting to {platformConfig.Host}:{GrpcPort}...");

        _streamTask = RunStreamAsync(client, _streamCts.Token);
    }

    private async Task RunStreamAsync(GrpcFaultClient client, CancellationToken token)
    {
        try
        {
            await foreach (var evt in client.StreamFaultsAsync(token))
            {
                // gRPC callback thread -- ObservableCollection updates must reach the UI
                // thread. WPF's default collection-changed binding does not auto-marshal,
                // so we dispatch explicitly here to avoid a cross-thread exception.
                System.Windows.Application.Current.Dispatcher.Invoke(() =>
                {
                    Events.Insert(0, evt); // newest first
                    OnPropertyChanged(nameof(EventCount));
                });
            }
        }
        catch (OperationCanceledException)
        {
            // Expected on disconnect -- not an error.
        }
        catch (Exception ex)
        {
            System.Windows.Application.Current.Dispatcher.Invoke(() =>
                AppendLog($"[ERROR] Stream error: {ex.Message}"));
        }
        finally
        {
            System.Windows.Application.Current.Dispatcher.Invoke(() =>
            {
                IsConnected = false;
                AppendLog("[INFO] Disconnected.");
            });
        }
    }

    [RelayCommand]
    private void Disconnect()
    {
        _streamCts?.Cancel();
        _streamCts?.Dispose();
        _streamCts = null;
        IsConnected = false;
    }

    [RelayCommand]
    private void Clear()
    {
        Events.Clear();
        OnPropertyChanged(nameof(EventCount));
    }

    private void AppendLog(string line)
    {
        ConnectionLog = ConnectionLog == "Connection log..."
            ? line
            : ConnectionLog + Environment.NewLine + line;
    }

    public FaultMonitorViewModel(ConfigManager configManager)
    {
        _configManager = configManager;
    }

    public void Dispose()
    {
        _streamCts?.Cancel();
        _streamCts?.Dispose();
    }
}