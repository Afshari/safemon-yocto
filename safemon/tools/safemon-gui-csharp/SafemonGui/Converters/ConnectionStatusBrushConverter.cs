using System.Globalization;
using System.Windows.Data;
using System.Windows.Media;

namespace SafemonGui.Converters;

public class ConnectionStatusBrushConverter : IValueConverter
{
    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        => (value is true)
            ? new SolidColorBrush(Color.FromRgb(0x2E, 0xCC, 0x71))
            : new SolidColorBrush(Color.FromRgb(0x5F, 0x6A, 0x82));

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        => throw new NotImplementedException();
}
