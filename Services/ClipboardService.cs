// ImageProcessing/Services/ClipboardService.cs

using System.Windows;
using System.Windows.Media.Imaging;

namespace ImageProcessing.Services
{
    public class ClipboardService
    {
        public void SetImage(BitmapSource image)
        {
            if (image != null)
            {
                Clipboard.SetImage(image);
            }
        }

        public BitmapSource GetImage()
        {
            if (Clipboard.ContainsImage())
            {
                return Clipboard.GetImage();
            }
            return null;
        }

        public void Clear()
        {
            Clipboard.Clear();
        }
    }
}