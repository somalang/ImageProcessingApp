using ImageProcessing.Services;
using ImageProcessing.ViewModel;
using System.Windows;

namespace ImageProcessing.Views
{
    /// <summary>
    /// Interaction logic for LogWindow.xaml
    /// </summary>
    public partial class LogWindow : Window
    {
        public LogWindow(LogService logService)
        {
            InitializeComponent();
            DataContext = new LogViewModel(logService);
        }
    }
}
