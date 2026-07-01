using SafemonGui.Core;
using Xunit;

namespace SafemonGui.Tests.Unit;

public class SshManagerTests
{
    [Fact]
    public void Constructor_AcceptsValidParameters()
    {
        var manager = new SshManager("192.168.1.147", 22, "root", "");
        Assert.NotNull(manager);
    }

    [Fact]
    public void Constructor_AcceptsEmptyPassword()
    {
        // Passwordless root is the expected default auth mode for these devices.
        var manager = new SshManager("192.168.1.147", 22, "root", string.Empty);
        Assert.NotNull(manager);
    }
}