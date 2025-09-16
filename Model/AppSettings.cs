using System;

namespace ImageProcessing.Models
{
    public class AppSettings
    {
        public string DefaultImagePath { get; set; } = Environment.GetFolderPath(Environment.SpecialFolder.Desktop);
        public double DefaultGaussianSigma { get; set; } = 1.0;
        public int DefaultLaplacianKernelType { get; set; } = 1;
        public int DefaultBinarizationThreshold { get; set; } = 128;
        public int DefaultDilationKernelSize { get; set; } = 3;
        public int DefaultErosionKernelSize { get; set; } = 3;
        public int DefaultMedianKernelSize { get; set; } = 3;
    }
}