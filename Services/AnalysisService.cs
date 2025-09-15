using ImageProcessing.Models;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Media.Imaging;

namespace ImageProcessing.Services
{
    public class AnalysisService
    {
        public string GenerateReport(BitmapImage image, IEnumerable<LogEntry> logs)
        {
            var reportBuilder = new StringBuilder();

            // 1. 기본 이미지 정보
            reportBuilder.AppendLine("## 이미지 분석 보고서");
            reportBuilder.AppendLine("---");
            reportBuilder.AppendLine($"**이미지 크기:** {image.PixelWidth} x {image.PixelHeight}");
            reportBuilder.AppendLine($"**DPI:** {image.DpiX} x {image.DpiY}");
            reportBuilder.AppendLine($"**포맷:** {image.Format}");
            reportBuilder.AppendLine();

            // 2. 처리 로그 요약
            reportBuilder.AppendLine("## 적용된 필터 및 처리 내역");
            reportBuilder.AppendLine("---");

            if (logs == null || !logs.Any())
            {
                reportBuilder.AppendLine("- 적용된 필터가 없습니다.");
            }
            else
            {
                // 각 필터의 적용 횟수 계산
                var filterCounts = logs.GroupBy(log => log.Operation)
                                       .Select(group => new { Operation = group.Key, Count = group.Count() })
                                       .OrderByDescending(x => x.Count);

                foreach (var filter in filterCounts)
                {
                    reportBuilder.AppendLine($"- **{filter.Operation}:** {filter.Count}회");
                }

                // 총 처리 시간 계산
                long totalProcessingTime = logs.Sum(log => log.ProcessingTimeMs);
                reportBuilder.AppendLine($"\n**총 처리 시간:** {totalProcessingTime} ms");
            }

            return reportBuilder.ToString();
        }
    }
}