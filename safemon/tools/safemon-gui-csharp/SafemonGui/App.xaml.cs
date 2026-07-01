using System.Windows;
using System.Windows.Threading;
using Microsoft.Extensions.DependencyInjection;
using SafemonGui.Core;
using SafemonGui.ViewModels;

namespace SafemonGui;

public partial class App : Application
{
    public static IServiceProvider Services { get; private set; } = null!;

    public App()
    {
        DispatcherUnhandledException += App_DispatcherUnhandledException;
        AppDomain.CurrentDomain.UnhandledException += CurrentDomain_UnhandledException;
    }

    private void App_DispatcherUnhandledException(object sender, DispatcherUnhandledExceptionEventArgs e)
    {
        MessageBox.Show(e.Exception.ToString(), "Unhandled Exception (Dispatcher)");
        e.Handled = true;
    }

    private void CurrentDomain_UnhandledException(object sender, UnhandledExceptionEventArgs e)
    {
        MessageBox.Show(e.ExceptionObject.ToString(), "Unhandled Exception (AppDomain)");
    }

    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        try
        {
            var services = new ServiceCollection();
            ConfigureServices(services);
            Services = services.BuildServiceProvider();

            var mainWindow = Services.GetRequiredService<MainWindow>();
            mainWindow.Show();
        }
        catch (Exception ex)
        {
            MessageBox.Show(ex.ToString(), "Startup Failed");
        }
    }

    private static void ConfigureServices(IServiceCollection services)
    {
        services.AddSingleton<ConfigManager>();

        services.AddSingleton<Func<string, int, string, string, SshManager>>(
            _ => (host, port, user, password) => new SshManager(host, port, user, password));

        services.AddSingleton<MainWindowViewModel>();
        services.AddTransient<KeyManagementViewModel>();

        services.AddSingleton<MainWindow>();
    }
}