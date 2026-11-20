using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace StarshatterWarsLauncher;

internal sealed class PilotRecoveryWindow : Window
{
    internal PilotRecovery.Checkpoint? Selected { get; private set; }
    internal PilotRecoveryWindow(IEnumerable<PilotRecovery.Checkpoint> checkpoints)
    {
        Title = "Recover Pilot Save"; Width = 780; Height = 380; MinWidth = 640; MinHeight = 300;
        WindowStartupLocation = WindowStartupLocation.CenterOwner;
        Background = new SolidColorBrush(Color.FromRgb(11, 20, 32)); Foreground = Brushes.White;
        var panel = new DockPanel { Margin = new Thickness(20) }; Content = panel;
        var intro = new TextBlock { Text = "Choose a checkpoint. Dates use your local time. Recovery replaces this pilot’s progress and its saved world, economy and skills. All current saves are backed up first.", TextWrapping = TextWrapping.Wrap, Margin = new Thickness(0, 0, 0, 14) };
        DockPanel.SetDock(intro, Dock.Top); panel.Children.Add(intro);
        var actions = new StackPanel { Orientation = Orientation.Horizontal, HorizontalAlignment = HorizontalAlignment.Right, Margin = new Thickness(0, 14, 0, 0) };
        DockPanel.SetDock(actions, Dock.Bottom); panel.Children.Add(actions);
        var cancel = new Button { Content = "Cancel", IsCancel = true, Padding = new Thickness(18, 8, 18, 8) };
        var restore = new Button { Content = "Back Up and Restore", IsEnabled = false, Padding = new Thickness(18, 8, 18, 8), Margin = new Thickness(8, 0, 0, 0) };
        actions.Children.Add(cancel); actions.Children.Add(restore);
        var list = new ListBox { ItemsSource = checkpoints.OrderByDescending(c => c.SavedUtc).ToArray(), DisplayMemberPath = "Label", Background = new SolidColorBrush(Color.FromRgb(18, 33, 49)), Foreground = Brushes.White };
        panel.Children.Add(list);
        list.SelectionChanged += (_, _) => restore.IsEnabled = list.SelectedItem is PilotRecovery.Checkpoint;
        restore.Click += (_, _) => { Selected = list.SelectedItem as PilotRecovery.Checkpoint; DialogResult = Selected is not null; };
    }
}
