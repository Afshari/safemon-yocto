using System.Globalization;
using System.Windows.Data;
using System.Windows.Media;
using SafemonGui.Models;

namespace SafemonGui.Converters;

public class ServiceStatusToBrushConverter : IValueConverter
{
    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        => (value as ServiceStatus?) switch
        {
            ServiceStatus.Ok => new SolidColorBrush(Color.FromRgb(0x2E, 0xCC, 0x71)),
            ServiceStatus.Warning => new SolidColorBrush(Color.FromRgb(0xF3, 0x9C, 0x12)),
            ServiceStatus.Error => new SolidColorBrush(Color.FromRgb(0xE7, 0x4C, 0x3C)),
            _ => new SolidColorBrush(Color.FromRgb(0x5F, 0x6A, 0x82)),
        };

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        => throw new NotImplementedException();
}