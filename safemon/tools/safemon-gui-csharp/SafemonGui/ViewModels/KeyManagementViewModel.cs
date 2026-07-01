using System.IO;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using SafemonGui.Core;
using SafemonGui.Crypto;

namespace SafemonGui.ViewModels;

public partial class KeyManagementViewModel : ObservableObject
{
    private readonly ConfigManager _configManager;
    private readonly Func<string, int, string, string, SshManager> _sshManagerFactory;

    [ObservableProperty]
    private string keyOutputDir;

    [ObservableProperty]
    private string publicKeyToUploadPath = string.Empty;

    [ObservableProperty]
    private string outputLog = "Output will appear here...";

    [ObservableProperty]
    private bool isBusy;

    public string DestinationOnDevice => "/etc/safemon/pki/safemon.pub";

    // Injected from MainWindowViewModel via binding in the page, not constructor,
    // since target/user/pass are shared state owned by the shell.
    public string Target { get; set; } = "raspberry_pi";
    public string Username { get; set; } = "root";
    public string Password { get; set; } = string.Empty;

    public KeyManagementViewModel(ConfigManager configManager,
        Func<string, int, string, string, SshManager> sshManagerFactory)
    {
        _configManager = configManager;
        _sshManagerFactory = sshManagerFactory;

        var appSettings = _configManager.LoadAppSettings();
        keyOutputDir = appSettings.KeyDir;
    }

    [RelayCommand]
    private void BrowseOutputDir()
    {
        var dialog = new Microsoft.Win32.OpenFolderDialog
        {
            InitialDirectory = Directory.Exists(KeyOutputDir) ? KeyOutputDir : Environment.CurrentDirectory
        };

        if (dialog.ShowDialog() == true)
            KeyOutputDir = dialog.FolderName;
    }

    [RelayCommand]
    private void BrowsePublicKeyToUpload()
    {
        var dialog = new Microsoft.Win32.OpenFileDialog
        {
            Filter = "Public key files (*.pub)|*.pub|All files (*.*)|*.*",
            InitialDirectory = Directory.Exists(KeyOutputDir) ? KeyOutputDir : Environment.CurrentDirectory
        };

        if (dialog.ShowDialog() == true)
            PublicKeyToUploadPath = dialog.FileName;
    }

    [RelayCommand]
    private async Task GenerateKeyPairAsync()
    {
        IsBusy = true;
        try
        {
            await Task.Run(() =>
            {
                var privateKey = Secp256k1.RandomScalar();
                var publicKey = privateKey * Secp256k1.G;

                if (!publicKey.IsOnCurve())
                {
                    AppendLog("[ERROR] Generated public key is not on curve -- this should never happen.");
                    return;
                }

                var keyPath = Path.Combine(KeyOutputDir, "safemon.key");
                var pubPath = Path.Combine(KeyOutputDir, "safemon.pub");

                KeyFileFormat.SaveKeyPair(keyPath, privateKey, publicKey);
                KeyFileFormat.SavePublicKey(pubPath, publicKey);

                AppendLog($"[OK] Private key written to: {keyPath}");
                AppendLog($"[OK] Public key written to:  {pubPath}");
                AppendLog($"[OK] Copy {pubPath} to /etc/safemon/pki/ on the device.");

                PublicKeyToUploadPath = pubPath;
            });
        }
        catch (Exception ex)
        {
            AppendLog($"[ERROR] Key generation failed: {ex.Message}");
        }
        finally
        {
            IsBusy = false;
        }
    }

    [RelayCommand]
    private async Task CopyToDeviceAsync()
    {
        if (string.IsNullOrWhiteSpace(PublicKeyToUploadPath) || !File.Exists(PublicKeyToUploadPath))
        {
            AppendLog("[ERROR] Select a public key file to copy first.");
            return;
        }

        IsBusy = true;
        try
        {
            var platformConfig = _configManager.LoadPlatformConfig(Target);
            var ssh = _sshManagerFactory(platformConfig.Host, platformConfig.Port, Username, Password);

            await ssh.UploadFileAsync(PublicKeyToUploadPath, DestinationOnDevice);

            AppendLog($"[OK] Copied {Path.GetFileName(PublicKeyToUploadPath)} to {Target}:{DestinationOnDevice}");
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
        OutputLog = OutputLog == "Output will appear here..."
            ? line
            : OutputLog + Environment.NewLine + line;
    }
}