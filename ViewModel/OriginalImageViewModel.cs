using System.Windows.Media.Imaging;

namespace ImageProcessing.ViewModels
{
    public class OriginalImageViewModel
    {
        public BitmapImage ImageToShow { get; }

        public OriginalImageViewModel(BitmapImage image)
        {
            ImageToShow = image;
        }
    }
}