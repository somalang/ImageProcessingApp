// Views/AnalysisReportWindow.xaml.cs

using ImageProcessing.ViewModel;
using System.Windows;

namespace ImageProcessing.Views
{
    public partial class AnalysisReportWindow : Window
    {
        public AnalysisReportWindow(string reportContent)
        {
            InitializeComponent();
            DataContext = new AnalysisReportViewModel(reportContent);
        }
    }
}