using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;

namespace SafemonGui.ViewModels;

public partial class MainWindowViewModel : ObservableObject
{
    private static readonly string[] PageNames =
    {
        "Key Management", "Sign / Verify", "Fault Monitor",
        "Device Files", "Device Status", "Settings"
    };

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

    public string[] Targets { get; } = { "raspberry_pi", "jetson_orin_nano", "qemu" };

    public string PageTitle => $"Safemon  \u2192  {PageNames[CurrentPageIndex]}";

    partial void OnCurrentPageIndexChanged(int value) => OnPropertyChanged(nameof(PageTitle));

    [RelayCommand]
    private void ToggleSidebar() => IsSidebarExpanded = !IsSidebarExpanded;

    [RelayCommand]
    private void NavigateTo(string indexStr)
    {
        if (int.TryParse(indexStr, out var index))
            CurrentPageIndex = index;
    }
}