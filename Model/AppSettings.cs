using System;

namespace ImageProcessing.Models
{
    public class AppSettings
    {
        public string DefaultImagePath { get; set; } = Environment.GetFolderPath(Environment.SpecialFolder.Desktop);

        // 필터별 기본값 추가
        public double DefaultGaussianSigma { get; set; } = 1.0;
        public int DefaultLaplacianKernelType { get; set; } = 1; // 0: 4-connectivity, 1: 8-connectivity
        public int DefaultBinarizationThreshold { get; set; } = 128;
        public int DefaultDilationKernelSize { get; set; } = 3;
        public int DefaultErosionKernelSize { get; set; } = 3;
        public int DefaultMedianKernelSize { get; set; } = 3;
    }
}