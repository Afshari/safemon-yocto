using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Microsoft.Extensions.DependencyInjection;

namespace SafemonGui.ViewModels;

public partial class MainWindowViewModel : ObservableObject
{
    private static readonly string[] PageNames =
    {
        "Key Management", "Sign / Verify", "Fault Monitor",
        "Device Files", "Device Status", "Settings"
    };

    private readonly IServiceProvider _serviceProvider;

    [ObservableProperty]
    private int currentPageIndex;

    [ObservableProperty]
    private bool isSidebarExpanded;

    [ObservableProperty]
    private string currentTarget = "raspberry_pi";

    [ObservableProperty]
    private string currentUsername = "root";

    [ObservableProperty]
    private string currentPassword = string.Empty;

    [ObservableProperty]
    private object? currentPageViewModel;

    public string[] Targets { get; } = { "raspberry_pi", "jetson_orin_nano", "qemu" };

    public string PageTitle => $"Safemon  \u2192  {PageNames[CurrentPageIndex]}";

    public MainWindowViewModel(IServiceProvider serviceProvider)
    {
        _serviceProvider = serviceProvider;
        UpdateCurrentPageViewModel();
    }

    partial void OnCurrentPageIndexChanged(int value)
    {
        OnPropertyChanged(nameof(PageTitle));
        UpdateCurrentPageViewModel();
    }

    private void UpdateCurrentPageViewModel()
    {
        CurrentPageViewModel = CurrentPageIndex switch
        {
            0 => CreateKeyManagementViewModel(),
            _ => null // other pages not wired in yet
        };
    }

    private KeyManagementViewModel CreateKeyManagementViewModel()
    {
        var vm = _serviceProvider.GetRequiredService<KeyManagementViewModel>();
        vm.Target = CurrentTarget;
        vm.Username = CurrentUsername;
        vm.Password = CurrentPassword;
        return vm;
    }

    [RelayCommand]
    private void ToggleSidebar() => IsSidebarExpanded = !IsSidebarExpanded;

    [RelayCommand]
    private void NavigateTo(string indexStr)
    {
        if (int.TryParse(indexStr, out var index))
            CurrentPageIndex = index;
    }
}