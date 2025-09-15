// ViewModel/AnalysisReportViewModel.cs

using ImageProcessing.ViewModels;

namespace ImageProcessing.ViewModel
{
    public class AnalysisReportViewModel : ViewModelBase
    {
        private string _reportContent;

        public string ReportContent
        {
            get => _reportContent;
            set => SetProperty(ref _reportContent, value);
        }

        public AnalysisReportViewModel(string reportContent)
        {
            ReportContent = reportContent;
        }
    }
}