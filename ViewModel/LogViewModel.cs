using ImageProcessing.Models;
using ImageProcessing.Services;
using ImageProcessing.ViewModels;
using System.Collections.ObjectModel;

namespace ImageProcessing.ViewModel
{
    public class LogViewModel : ViewModelBase
    {
        private readonly LogService _logService;
        public ObservableCollection<LogEntry> LogHistory => _logService.LogHistory;
        public LogViewModel(LogService logService)
        {
            _logService = logService;
        }
    }
}
