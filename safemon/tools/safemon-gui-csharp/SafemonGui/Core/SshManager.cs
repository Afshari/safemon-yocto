using System.IO;
using System.Net.Sockets;
using Renci.SshNet;
using Renci.SshNet.Common;

namespace SafemonGui.Core;

public record CommandResult
{
    public required int ExitCode { get; init; }
    public required string Output { get; init; }
    public required string Error { get; init; }
    public bool Succeeded => ExitCode == 0;
}

/// SSH/SFTP wrapper. Connects, performs one operation, disconnects -- no persistent
/// connection is kept between calls, matching ssh_manager.py's approach.
public class SshManager
{
    private readonly string _host;
    private readonly int _port;
    private readonly string _user;
    private readonly string _password;

    public SshManager(string host, int port, string user, string password)
    {
        _host = host;
        _port = port;
        _user = user;
        _password = password;
    }

    private ConnectionInfo BuildConnectionInfo()
    {
        // Passwordless root is the primary auth mode for these devices; an empty
        // password string still works as a fallback via SSH.NET's PasswordAuthenticationMethod.
        var authMethods = new List<AuthenticationMethod>
        {
            new PasswordAuthenticationMethod(_user, _password ?? string.Empty)
        };

        return new ConnectionInfo(_host, _port, _user, authMethods.ToArray());
    }

    public async Task<CommandResult> ExecuteCommandAsync(string command, CancellationToken cancellationToken = default)
    {
        using var client = new SshClient(BuildConnectionInfo());

        return await Task.Run(() =>
        {
            client.Connect();
            try
            {
                using var cmd = client.CreateCommand(command);
                var output = cmd.Execute();

                return new CommandResult
                {
                    ExitCode = cmd.ExitStatus ?? -1,
                    Output = output,
                    Error = cmd.Error ?? string.Empty
                };
            }
            finally
            {
                client.Disconnect();
            }
        }, cancellationToken);
    }

    public async Task UploadFileAsync(string localPath, string remotePath, CancellationToken cancellationToken = default)
    {
        var remotePathUnix = ToUnixPath(remotePath);

        using var client = new SftpClient(BuildConnectionInfo());

        await Task.Run(() =>
        {
            client.Connect();
            try
            {
                EnsureRemoteDirectoryExists(client, remotePathUnix);

                using var stream = File.OpenRead(localPath);
                client.UploadFile(stream, remotePathUnix, canOverride: true);
            }
            finally
            {
                client.Disconnect();
            }
        }, cancellationToken);
    }

    public async Task DownloadFileAsync(string remotePath, string localPath, CancellationToken cancellationToken = default)
    {
        var remotePathUnix = ToUnixPath(remotePath);

        using var client = new SftpClient(BuildConnectionInfo());

        await Task.Run(() =>
        {
            client.Connect();
            try
            {
                var dir = Path.GetDirectoryName(Path.GetFullPath(localPath));
                if (!string.IsNullOrEmpty(dir))
                    Directory.CreateDirectory(dir);

                using var stream = File.Create(localPath);
                client.DownloadFile(remotePathUnix, stream);
            }
            finally
            {
                client.Disconnect();
            }
        }, cancellationToken);
    }

    public async Task<string> ReadRemoteFileAsync(string remotePath, CancellationToken cancellationToken = default)
    {
        var remotePathUnix = ToUnixPath(remotePath);

        using var client = new SftpClient(BuildConnectionInfo());

        return await Task.Run(() =>
        {
            client.Connect();
            try
            {
                return client.ReadAllText(remotePathUnix);
            }
            finally
            {
                client.Disconnect();
            }
        }, cancellationToken);
    }

    public async Task<bool> TestConnectionAsync(CancellationToken cancellationToken = default)
    {
        try
        {
            using var client = new SshClient(BuildConnectionInfo());
            await Task.Run(() => client.Connect(), cancellationToken);
            var connected = client.IsConnected;
            client.Disconnect();
            return connected;
        }
        catch (SshAuthenticationException)
        {
            return false;
        }
        catch (SshConnectionException)
        {
            return false;
        }
        catch (SocketException)
        {
            return false;
        }
    }

    private static void EnsureRemoteDirectoryExists(SftpClient client, string remotePath)
    {
        var dir = remotePath.Contains('/')
            ? remotePath[..remotePath.LastIndexOf('/')]
            : string.Empty;

        if (string.IsNullOrEmpty(dir))
            return;

        var parts = dir.Split('/', StringSplitOptions.RemoveEmptyEntries);
        var current = dir.StartsWith('/') ? "/" : string.Empty;

        foreach (var part in parts)
        {
            current = current.EndsWith('/') ? current + part : current + "/" + part;
            if (!client.Exists(current))
                client.CreateDirectory(current);
        }
    }

    /// All remote paths use forward slashes regardless of the host OS this app runs on.
    private static string ToUnixPath(string path) => path.Replace('\\', '/');
}