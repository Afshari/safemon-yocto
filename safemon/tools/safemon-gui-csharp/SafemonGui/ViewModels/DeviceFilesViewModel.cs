using System.IO;
using System.Text;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using SafemonGui.Core;
using SafemonGui.Models;

namespace SafemonGui.ViewModels;

public partial class DeviceFilesViewModel : ObservableObject
{
    private readonly ConfigManager _configManager;
    private readonly Func<string, int, string, string, SshManager> _sshManagerFactory;

    // Extensions treated as text for line-ending conversion; everything else sent as-is.
    private static readonly HashSet<string> TextExtensions = new(StringComparer.OrdinalIgnoreCase)
    {
        ".sh", ".conf", ".txt", ".py", ".json"
    };

    public string Target { get; set; } = "raspberry_pi";
    public string Username { get; set; } = "root";
    public string Password { get; set; } = string.Empty;

    public List<DeviceFileEntry> KnownFiles { get; private set; } = new();

    [ObservableProperty]
    private DeviceFileEntry? selectedKnownFile;

    [ObservableProperty]
    private string knownFileLocalPath = "-";

    [ObservableProperty]
    private string transferLocalPath = string.Empty;

    [ObservableProperty]
    private string transferDestinationPath = string.Empty;

    [ObservableProperty]
    private string filePreview = "File preview (read-only)...";

    [ObservableProperty]
    private string statusLog = "Status messages will appear here...";

    [ObservableProperty]
    private bool isBusy;

    public DeviceFilesViewModel(ConfigManager configManager,
        Func<string, int, string, string, SshManager> sshManagerFactory)
    {
        _configManager = configManager;
        _sshManagerFactory = sshManagerFactory;

        KnownFiles = _configManager.LoadDeviceFiles();
        SelectedKnownFile = KnownFiles.FirstOrDefault();
    }

    private SshManager CreateSshManager()
    {
        var platformConfig = _configManager.LoadPlatformConfig(Target);
        return _sshManagerFactory(platformConfig.Host, platformConfig.Port, Username, Password);
    }

    [RelayCommand]
    private void BrowseKnownFileLocal()
    {
        var dialog = new Microsoft.Win32.OpenFileDialog();
        if (dialog.ShowDialog() == true)
            KnownFileLocalPath = dialog.FileName;
    }

    [RelayCommand]
    private async Task LoadFromDeviceAsync()
    {
        if (SelectedKnownFile is null)
        {
            AppendLog("[ERROR] Select a known file first.");
            return;
        }

        IsBusy = true;
        try
        {
            var ssh = CreateSshManager();
            var content = await ssh.ReadRemoteFileAsync(SelectedKnownFile.DevicePath);
            FilePreview = content;
            AppendLog($"[OK] Loaded from device: {SelectedKnownFile.DevicePath}");
        }
        catch (Exception ex)
        {
            AppendLog($"[ERROR] Load from device failed: {ex.Message}");
        }
        finally
        {
            IsBusy = false;
        }
    }

    [RelayCommand]
    private async Task CopyKnownFileToDeviceAsync()
    {
        if (SelectedKnownFile is null)
        {
            AppendLog("[ERROR] Select a known file first.");
            return;
        }

        if (string.IsNullOrWhiteSpace(KnownFileLocalPath) || KnownFileLocalPath == "-" || !File.Exists(KnownFileLocalPath))
        {
            AppendLog("[ERROR] Browse to a local file first.");
            return;
        }

        // Confirmation before overwriting, matching Python behavior.
        var result = System.Windows.MessageBox.Show(
            $"Overwrite {SelectedKnownFile.DevicePath} on {Target}?",
            "Confirm Overwrite",
            System.Windows.MessageBoxButton.YesNo,
            System.Windows.MessageBoxImage.Warning);

        if (result != System.Windows.MessageBoxResult.Yes)
            return;

        await CopyFileToDeviceAsync(KnownFileLocalPath, SelectedKnownFile.DevicePath);
    }

    [RelayCommand]
    private void BrowseTransferLocal()
    {
        var dialog = new Microsoft.Win32.OpenFileDialog();
        if (dialog.ShowDialog() == true)
            TransferLocalPath = dialog.FileName;
    }

    [RelayCommand]
    private async Task CopyTransferToDeviceAsync()
    {
        if (string.IsNullOrWhiteSpace(TransferLocalPath) || !File.Exists(TransferLocalPath))
        {
            AppendLog("[ERROR] Browse to a local file first.");
            return;
        }

        if (string.IsNullOrWhiteSpace(TransferDestinationPath))
        {
            AppendLog("[ERROR] Enter a destination path on device.");
            return;
        }

        var result = System.Windows.MessageBox.Show(
            $"Overwrite {TransferDestinationPath} on {Target}?",
            "Confirm Overwrite",
            System.Windows.MessageBoxButton.YesNo,
            System.Windows.MessageBoxImage.Warning);

        if (result != System.Windows.MessageBoxResult.Yes)
            return;

        await CopyFileToDeviceAsync(TransferLocalPath, TransferDestinationPath);
    }

    private async Task CopyFileToDeviceAsync(string localPath, string devicePath)
    {
        IsBusy = true;
        try
        {
            // Copy to Device always reads from local disk, never from the preview text box.
            var uploadPath = localPath;
            var isText = TextExtensions.Contains(Path.GetExtension(localPath));

            if (isText)
            {
                // Automatic Unix line ending conversion for text files before transfer.
                var content = await File.ReadAllTextAsync(localPath);
                var converted = content.Replace("\r\n", "\n");

                if (converted != content)
                {
                    var tempPath = Path.GetTempFileName();
                    await File.WriteAllTextAsync(tempPath, converted, new UTF8Encoding(encoderShouldEmitUTF8Identifier: false));
                    uploadPath = tempPath;
                }
            }

            var ssh = CreateSshManager();
            await ssh.UploadFileAsync(uploadPath, devicePath);

            if (uploadPath != localPath)
                File.Delete(uploadPath); // clean up temp file used for line-ending conversion

            AppendLog($"[OK] Copied {Path.GetFileName(localPath)} -> {Target}:{devicePath}");
        }
        catch (Exception ex)
        {
            AppendLog($"[ERROR] Copy to device failed: {ex.Message}");
        }
        finally
        {
            IsBusy = false;
        }
    }

    private void AppendLog(string line)
    {
        StatusLog = StatusLog == "Status messages will appear here..."
            ? line
            : StatusLog + Environment.NewLine + line;
    }
}