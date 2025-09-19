using ImageProcessing.Models;
using Microsoft.Win32;
using System;
using System.IO;
using System.Threading.Tasks;
using System.Windows.Media.Imaging;

namespace ImageProcessing.Services
{
    public class FileService
    {
        public string OpenImageFileDialog()
        {
            var dialog = new OpenFileDialog
            {
                Filter = "Image Files|*.jpg;*.png;*.bmp;*.jpeg"
            };

            if (dialog.ShowDialog() == true)
            {
                return dialog.FileName;
            }
            return null;
        }

        public async Task<BitmapImage> LoadImage(string filePath)
        {
            return await Task.Run(() =>
            {
                var image = new BitmapImage();
                image.BeginInit();
                image.CacheOption = BitmapCacheOption.OnLoad;
                image.UriSource = new Uri(filePath);
                image.EndInit();
                image.Freeze();
                return image;
            });
        }

        public string SaveImageFileDialog()
        {
            var dialog = new SaveFileDialog
            {
                Filter = "PNG Image|*.png|JPEG Image|*.jpg"
            };

            if (dialog.ShowDialog() == true)
            {
                return dialog.FileName;
            }
            return null;
        }

        public async Task SaveImage(BitmapImage image, string filePath)
        {
            await Task.Run(() =>
            {
                using var fileStream = new FileStream(filePath, FileMode.Create);
                var encoder = new PngBitmapEncoder();
                encoder.Frames.Add(BitmapFrame.Create(image));
                encoder.Save(fileStream);
            });
        }
    }
}