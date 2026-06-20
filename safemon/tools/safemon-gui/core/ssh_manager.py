"""
ssh_manager.py - SSH/SCP connection manager using Paramiko.
No credentials are ever written to disk - username/password live
only in memory for the duration of a connection.
"""

import paramiko


class SSHManager:
    """
    Wraps a single SSH connection to a device.

    Usage:
        mgr = SSHManager(host="192.168.1.42", port=22, username="root", password="")
        mgr.connect()
        output = mgr.run_command("systemctl status safemon-app")
        mgr.put_file("local/path", "/remote/path")
        mgr.get_file("/remote/path", "local/path")
        mgr.close()
    """

    def __init__(self, host, port, username, password=""):
        self.host = host
        self.port = port
        self.username = username
        self.password = password if password else None
        self._client = None

    def connect(self, timeout=8):
        self._client = paramiko.SSHClient()
        self._client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

        try:
            self._client.connect(
                hostname=self.host,
                port=self.port,
                username=self.username,
                password=self.password,
                timeout=timeout,
                look_for_keys=True,
                allow_agent=True,
            )
            return
        except paramiko.ssh_exception.SSHException:
            pass

        # Fall back: try empty password explicitly, ignoring local key files
        self._client.connect(
            hostname=self.host,
            port=self.port,
            username=self.username,
            password="",
            timeout=timeout,
            look_for_keys=False,
            allow_agent=False,
        )

    def is_connected(self):
        return self._client is not None and self._client.get_transport() is not None \
            and self._client.get_transport().is_active()

    def run_command(self, command, timeout=15):
        """
        Run a command over SSH and return (exit_code, stdout, stderr).
        """
        if not self.is_connected():
            raise RuntimeError("Not connected. Call connect() first.")

        stdin, stdout, stderr = self._client.exec_command(command, timeout=timeout)
        exit_code = stdout.channel.recv_exit_status()
        out = stdout.read().decode("utf-8", errors="replace")
        err = stderr.read().decode("utf-8", errors="replace")
        return exit_code, out, err

    def put_file(self, local_path, remote_path):
        """Copy a local file to the remote device via SFTP."""
        if not self.is_connected():
            raise RuntimeError("Not connected. Call connect() first.")
        sftp = self._client.open_sftp()
        try:
            sftp.put(local_path, remote_path)
        finally:
            sftp.close()

    def get_file(self, remote_path, local_path):
        """Copy a remote file to the local machine via SFTP."""
        if not self.is_connected():
            raise RuntimeError("Not connected. Call connect() first.")
        sftp = self._client.open_sftp()
        try:
            sftp.get(remote_path, local_path)
        finally:
            sftp.close()

    def close(self):
        if self._client is not None:
            self._client.close()
            self._client = None