using System;
using System.Windows.Media.Imaging;

namespace ImageProcessing.Models
{
    public class ImageModel
    {
        public BitmapImage OriginalImage { get; set; }
        public BitmapImage ProcessedImage { get; set; }
        public string FilePath { get; set; }
        public string FileName { get; set; }
        public DateTime LoadedTime { get; set; }
        public int Width { get; set; }
        public int Height { get; set; }
        public string Format { get; set; }
    }
}
