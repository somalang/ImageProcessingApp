namespace ImageProcessing.Models
{
    public class FilterParameters
    {
        public double GaussianSigma { get; set; } = 1.0;
        public int LaplacianKernelType { get; set; } = 1;
        public int BinarizationThreshold { get; set; } = 128;
        public int DilationKernelSize { get; set; } = 3;
        public int ErosionKernelSize { get; set; } = 3;
        public int MedianKernelSize { get; set; } = 3;
        public bool IsEnabled { get; set; } = true;
    }
}