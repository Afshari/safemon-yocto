using System.Globalization;
using System.Windows.Data;
using System.Windows.Media;

namespace SafemonGui.Converters;

public class FaultStatusToBrushConverter : IValueConverter
{
    public object Convert(object? value, Type targetType, object? parameter, CultureInfo culture)
        => (value as string) switch
        {
            "Ok" => new SolidColorBrush(Color.FromRgb(0x2E, 0xCC, 0x71)),
            "Warning" => new SolidColorBrush(Color.FromRgb(0xF3, 0x9C, 0x12)),
            "Error" => new SolidColorBrush(Color.FromRgb(0xE7, 0x4C, 0x3C)),
            _ => new SolidColorBrush(Color.FromRgb(0xC3, 0xCB, 0xDD)),
        };

    public object ConvertBack(object? value, Type targetType, object? parameter, CultureInfo culture)
        => throw new NotImplementedException();
}
