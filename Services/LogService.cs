using ImageProcessing.Models;
using System.Collections.ObjectModel;

namespace ImageProcessing.Services
{
    public class LogService
    {
        public ObservableCollection<LogEntry> LogHistory { get; } = new ObservableCollection<LogEntry>();

        public void AddLog(string operation, long time)
        {
            var entry = new LogEntry
            {
                Timestamp = System.DateTime.Now,
                Operation = operation,
                ProcessingTimeMs = time
            };
            LogHistory.Insert(0, entry); // 최신 로그가 맨 위에 오도록
        }
        public IEnumerable<LogEntry> GetLogs()
        {
            // _logEntries 대신 LogHistory를 반환
            return LogHistory;
        }
    }
}