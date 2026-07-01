using System.Globalization;
using System.Windows.Data;

namespace SafemonGui.Converters;

public class AutoRefreshLabelConverter : IValueConverter
{
    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        => (value is true) ? "Stop Auto-Refresh" : "Start Auto-Refresh";

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        => throw new NotImplementedException();
}