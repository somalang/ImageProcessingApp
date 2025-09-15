using System;

namespace ImageProcessing.Models
{
    public class LogEntry
    {
        public DateTime Timestamp { get; set; }
        public string Operation { get; set; }
        public long ProcessingTimeMs { get; set; }

        public override string ToString()
        {
            return $"[{Timestamp:HH:mm:ss}] {Operation} - {ProcessingTimeMs} ms";
        }
    }
}