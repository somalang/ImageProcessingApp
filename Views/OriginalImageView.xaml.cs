using System.Windows;
using System.Windows.Media.Imaging;
using ImageProcessing.ViewModels;

namespace ImageProcessing.Views
{
    public partial class OriginalImageView : Window
    {
        public OriginalImageView(BitmapImage image)
        {
            InitializeComponent();
            DataContext = new OriginalImageViewModel(image);
        }
    }
}