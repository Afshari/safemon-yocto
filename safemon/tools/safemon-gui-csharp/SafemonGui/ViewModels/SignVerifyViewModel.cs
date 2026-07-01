using System.IO;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using SafemonGui.Crypto;

namespace SafemonGui.ViewModels;

public partial class SignVerifyViewModel : ObservableObject
{
    [ObservableProperty]
    private string fileToSignPath = string.Empty;

    [ObservableProperty]
    private string privateKeyPath = string.Empty;

    [ObservableProperty]
    private string fileToVerifyPath = string.Empty;

    [ObservableProperty]
    private string signaturePath = string.Empty;

    [ObservableProperty]
    private string publicKeyPath = string.Empty;

    [ObservableProperty]
    private string resultLog = "Sign/verify results will appear here...";

    [ObservableProperty]
    private bool isBusy;

    [RelayCommand]
    private void BrowseFileToSign() => FileToSignPath = BrowseForFile("All files (*.*)|*.*");

    [RelayCommand]
    private void BrowsePrivateKey() => PrivateKeyPath = BrowseForFile("Private key files (*.key)|*.key|All files (*.*)|*.*");

    [RelayCommand]
    private void BrowseFileToVerify() => FileToVerifyPath = BrowseForFile("All files (*.*)|*.*");

    [RelayCommand]
    private void BrowseSignature() => SignaturePath = BrowseForFile("Signature files (*.sig)|*.sig|All files (*.*)|*.*");

    [RelayCommand]
    private void BrowsePublicKey() => PublicKeyPath = BrowseForFile("Public key files (*.pub)|*.pub|All files (*.*)|*.*");

    [RelayCommand]
    private async Task SignAsync()
    {
        if (string.IsNullOrWhiteSpace(FileToSignPath) || !File.Exists(FileToSignPath))
        {
            AppendLog("[ERROR] Select a file to sign.");
            return;
        }

        if (string.IsNullOrWhiteSpace(PrivateKeyPath) || !File.Exists(PrivateKeyPath))
        {
            AppendLog("[ERROR] Select a private key file.");
            return;
        }

        IsBusy = true;
        try
        {
            await Task.Run(() =>
            {
                var (priv, _) = KeyFileFormat.LoadKey(PrivateKeyPath);
                if (priv is null)
                {
                    AppendLog("[ERROR] Key file does not contain a private key.");
                    return;
                }

                var fileHash = KeyFileFormat.HashFile(FileToSignPath);
                var (r, s) = Ecdsa.Sign(fileHash, priv.Value);

                var sigPath = FileToSignPath + ".sig";
                KeyFileFormat.SaveSignature(sigPath, r, s);

                AppendLog($"[OK] Signed:     {FileToSignPath}");
                AppendLog($"[OK] Signature:  {sigPath}");
            });
        }
        catch (Exception ex)
        {
            AppendLog($"[ERROR] Signing failed: {ex.Message}");
        }
        finally
        {
            IsBusy = false;
        }
    }

    [RelayCommand]
    private async Task VerifyAsync()
    {
        if (string.IsNullOrWhiteSpace(FileToVerifyPath) || !File.Exists(FileToVerifyPath))
        {
            AppendLog("[ERROR] Select a file to verify.");
            return;
        }

        if (string.IsNullOrWhiteSpace(SignaturePath) || !File.Exists(SignaturePath))
        {
            AppendLog("[ERROR] Select a signature file.");
            return;
        }

        if (string.IsNullOrWhiteSpace(PublicKeyPath) || !File.Exists(PublicKeyPath))
        {
            AppendLog("[ERROR] Select a public key file.");
            return;
        }

        IsBusy = true;
        try
        {
            await Task.Run(() =>
            {
                var (_, pub) = KeyFileFormat.LoadKey(PublicKeyPath);
                var (r, s) = KeyFileFormat.LoadSignature(SignaturePath);
                var fileHash = KeyFileFormat.HashFile(FileToVerifyPath);

                var valid = Ecdsa.Verify(fileHash, r, s, pub);

                AppendLog(valid
                    ? $"[OK] Signature is valid for: {FileToVerifyPath}"
                    : $"[FAIL] Signature is INVALID for: {FileToVerifyPath}");
            });
        }
        catch (Exception ex)
        {
            AppendLog($"[ERROR] Verification failed: {ex.Message}");
        }
        finally
        {
            IsBusy = false;
        }
    }

    private static string BrowseForFile(string filter)
    {
        var dialog = new Microsoft.Win32.OpenFileDialog { Filter = filter };
        return dialog.ShowDialog() == true ? dialog.FileName : string.Empty;
    }

    private void AppendLog(string line)
    {
        ResultLog = ResultLog == "Sign/verify results will appear here..."
            ? line
            : ResultLog + Environment.NewLine + line;
    }
}