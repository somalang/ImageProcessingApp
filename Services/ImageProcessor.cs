using ImageProcessingWrapper;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace ImageProcessing.Services
{
    public class ImageProcessor
    {
        private readonly ImageEngine _engine = new ImageEngine();
        private readonly Stack<BitmapImage> _undoStack = new Stack<BitmapImage>();
        private readonly Stack<BitmapImage> _redoStack = new Stack<BitmapImage>();

        public bool CanUndo => _undoStack.Any();
        public bool CanRedo => _redoStack.Any();
        public bool HasFFTData() => _engine.HasFFTData();

        private void AddToUndoStack(BitmapImage image)
        {
            if (image != null)
            {
                _undoStack.Push(CloneBitmapImage(image)); // 복사본 저장하기
                _redoStack.Clear();
            }
        }

        private BitmapImage ProcessImage(BitmapImage source, Func<BitmapImage, BitmapImage> processFunction)
        {
            if (source == null) return null;

            AddToUndoStack(source); // 작업 전 원본 이미지를 Undo 스택에 추가
            return processFunction(source);
        }

        public BitmapSource Crop(BitmapSource source, Rect rect)
        {
            if (rect.IsEmpty || rect.Width == 0 || rect.Height == 0) return null;
            rect.Intersect(new Rect(0, 0, source.PixelWidth, source.PixelHeight));
            if (rect.IsEmpty) return null;

            return new CroppedBitmap(source, new Int32Rect((int)rect.X, (int)rect.Y, (int)rect.Width, (int)rect.Height));
        }

        public BitmapImage ClearSelection(BitmapImage source, Rect rect)
        {
            return ProcessImage(source, (img) =>
            {
                if (rect.IsEmpty || rect.Width <= 0 || rect.Height <= 0) return img;

                rect.Intersect(new Rect(0, 0, img.PixelWidth, img.PixelHeight));

                if (rect.Width <= 0 || rect.Height <= 0) return img;

                var writeableBitmap = new WriteableBitmap(new FormatConvertedBitmap(img, PixelFormats.Bgra32, null, 0));

                int selectionStride = (int)rect.Width * 4; // 선택 영역의 가로 한 줄에 해당하는 바이트 수
                byte[] transparentPixels = new byte[(int)rect.Height * selectionStride];

                writeableBitmap.WritePixels(
                    new Int32Rect((int)rect.X, (int)rect.Y, (int)rect.Width, (int)rect.Height), // 쓸 위치와 크기
                    transparentPixels,  // 쓸 데이터
                    selectionStride,    // 쓸 데이터의 stride
                    0);                 // 오프셋

                return ConvertBitmapSourceToBitmapImage(writeableBitmap);
            });
        }

        public BitmapImage Paste(BitmapImage destination, BitmapSource sourceToPaste, Point location)
        {
            return ProcessImage(destination, (dest) =>
            {
                var writeable = new WriteableBitmap(new FormatConvertedBitmap(dest, PixelFormats.Pbgra32, null, 0));
                var sourceFormatted = new FormatConvertedBitmap(sourceToPaste, PixelFormats.Pbgra32, null, 0);

                var renderTarget = new RenderTargetBitmap(writeable.PixelWidth, writeable.PixelHeight, 96, 96, PixelFormats.Pbgra32);
                var drawingVisual = new DrawingVisual();
                using (DrawingContext drawingContext = drawingVisual.RenderOpen())
                {
                    drawingContext.DrawImage(writeable, new Rect(0, 0, writeable.PixelWidth, writeable.PixelHeight));
                    drawingContext.DrawImage(sourceFormatted, new Rect(location.X, location.Y, sourceFormatted.PixelWidth, sourceFormatted.PixelHeight));
                }
                renderTarget.Render(drawingVisual);

                return ConvertBitmapSourceToBitmapImage(renderTarget);
            });
        }

        public Rect TemplateMatch(BitmapSource source, BitmapSource templateImage)
        {
            if (source == null || templateImage == null) return Rect.Empty;
            if (templateImage.PixelWidth > source.PixelWidth || templateImage.PixelHeight > source.PixelHeight)
            {
                MessageBox.Show("템플릿 이미지가 원본 이미지보다 큽니다.", "템플릿 매칭 오류", MessageBoxButton.OK, MessageBoxImage.Warning);
                return Rect.Empty;
            }

            var sourceBitmap = new FormatConvertedBitmap(source, PixelFormats.Bgra32, null, 0);
            int sourceStride = sourceBitmap.PixelWidth * 4;
            byte[] sourcePixels = new byte[sourceBitmap.PixelHeight * sourceStride];
            sourceBitmap.CopyPixels(sourcePixels, sourceStride, 0);

            var templateBitmap = new FormatConvertedBitmap(templateImage, PixelFormats.Bgra32, null, 0);
            int templateStride = templateBitmap.PixelWidth * 4;
            byte[] templatePixels = new byte[templateBitmap.PixelHeight * templateStride];
            templateBitmap.CopyPixels(templatePixels, templateStride, 0);
            int matchX = -1, matchY = -1;
            _engine.ApplyTemplateMatch(
                sourcePixels, source.PixelWidth, source.PixelHeight,
                templatePixels, templateImage.PixelWidth, templateImage.PixelHeight,
                ref matchX, ref matchY
            );

            if (matchX >= 0 && matchY >= 0)
            {
                return new Rect(matchX, matchY, templateImage.PixelWidth, templateImage.PixelHeight);
            }
            else
            {
                MessageBox.Show("유사한 영역을 찾지 못했습니다.", "템플릿 매칭 결과", MessageBoxButton.OK, MessageBoxImage.Information);
                return Rect.Empty;
            }
        }
        private BitmapImage ProcessImageInternal(BitmapImage img, Action<byte[], int, int> processAction)
        {
            var bitmap = new FormatConvertedBitmap(img, PixelFormats.Bgra32, null, 0);
            int width = bitmap.PixelWidth;
            int height = bitmap.PixelHeight;
            int stride = width * 4;
            byte[] pixels = new byte[height * stride];
            bitmap.CopyPixels(pixels, stride, 0);

            processAction(pixels, width, height);

            var processedBitmap = BitmapSource.Create(width, height, 96, 96, PixelFormats.Bgra32, null, pixels, stride);
            return ConvertBitmapSourceToBitmapImage(processedBitmap);
        }

        private BitmapImage ApplyFilter(BitmapImage source, Action<byte[], int, int> processAction)
        {
            return ProcessImage(source, delegate (BitmapImage img) {
                return ProcessImageInternal(img, processAction);
            });
        }


        public BitmapImage ApplyGrayscale(BitmapImage source) {
           return ApplyFilter(source, (p, w, h) => _engine.ApplyGrayscale(p, w, h));
        }
        public BitmapImage ApplyGaussianBlur(BitmapImage source, float sigma) { 
            return ApplyFilter(source, (p, w, h) => _engine.ApplyGaussianBlur(p, w, h, sigma));
        }
        public BitmapImage ApplySobel(BitmapImage source) { 
            return ApplyFilter(source, (p, w, h) => _engine.ApplySobel(p, w, h));
        }
        public BitmapImage ApplyLaplacian(BitmapImage source, int kernelType)
        {
            return ApplyFilter(source, (p, w, h) => _engine.ApplyLaplacian(p, w, h));
        }
        public BitmapImage ApplyBinarization(BitmapImage source, int param = 128)
        {
            return ApplyFilter(source, (p, w, h) => _engine.ApplyBinarization(p, w, h));
        }
        public BitmapImage ApplyDilation(BitmapImage source, int param = 3)
        {
            return ApplyFilter(source, (p, w, h) => _engine.ApplyDilation(p, w, h));
        }
        public BitmapImage ApplyErosion(BitmapImage source, int param = 3)
        {
            return ApplyFilter(source, (p, w, h) => _engine.ApplyErosion(p, w, h));
        }
        public BitmapImage ApplyMedian(BitmapImage source, int param = 3)
        {
            return ApplyFilter(source, (p, w, h) => _engine.ApplyMedian(p, w, h, param));
        }

        public BitmapImage ApplyFFT(BitmapImage source)
        {
            return ProcessImage(source, (img) => {
                var bitmap = new FormatConvertedBitmap(img, PixelFormats.Bgra32, null, 0);
                byte[] pixels = new byte[bitmap.PixelHeight * bitmap.PixelWidth * 4];
                bitmap.CopyPixels(pixels, bitmap.PixelWidth * 4, 0);

                if (!_engine.ApplyFFT(pixels, bitmap.PixelWidth, bitmap.PixelHeight))
                {
                    MessageBox.Show("FFT 처리 중 오류.", "오류", MessageBoxButton.OK, MessageBoxImage.Error);
                    _undoStack.Pop();
                    return img; // 원본 반환
                }

                return ConvertBitmapSourceToBitmapImage(BitmapSource.Create(bitmap.PixelWidth, bitmap.PixelHeight, 96, 96, PixelFormats.Bgra32, null, pixels, bitmap.PixelWidth * 4));
            });
        }

        public BitmapImage ApplyIFFT(BitmapImage source)
        {
            if (!HasFFTData())
            {
                MessageBox.Show("FFT 데이터가 없습니다.", "경고", MessageBoxButton.OK, MessageBoxImage.Warning);
                return source;
            }
            return ProcessImage(source, (img) => {
                var bitmap = new FormatConvertedBitmap(img, PixelFormats.Bgra32, null, 0);
                byte[] pixels = new byte[bitmap.PixelHeight * bitmap.PixelWidth * 4];

                if (!_engine.ApplyIFFT(pixels, bitmap.PixelWidth, bitmap.PixelHeight))
                {
                    MessageBox.Show("IFFT 처리 중 오류.", "오류", MessageBoxButton.OK, MessageBoxImage.Error);
                    _undoStack.Pop();
                    return img;
                }

                return ConvertBitmapSourceToBitmapImage(BitmapSource.Create(bitmap.PixelWidth, bitmap.PixelHeight, 96, 96, PixelFormats.Bgra32, null, pixels, bitmap.PixelWidth * 4));
            });
        }
        // 프로세스 내부냐 아니면 사용자 접근 여부를 따지는 거냐 ..어쩌구

        public void ClearFFTData() => _engine.ClearFFTData();

        public BitmapImage Undo(BitmapImage currentImage)
        {
            if (CanUndo)
            {
                _redoStack.Push(CloneBitmapImage(currentImage));
                return _undoStack.Pop();
            }
            return currentImage;
        }

        public BitmapImage Redo(BitmapImage currentImage)
        {
            if (CanRedo)
            {
                _undoStack.Push(CloneBitmapImage(currentImage));
                return _redoStack.Pop();
            }
            return currentImage;
        }

        private BitmapImage ConvertBitmapSourceToBitmapImage(BitmapSource source)
        {
            if (source == null) return null;

            // 소스가 이미 BitmapImage이고 스트림 소스가 없는 경우 (메모리에 이미 있는 경우) 바로 반환
            if (source is BitmapImage bitmapImage && bitmapImage.StreamSource == null)
            {
                return bitmapImage;
            }

            // PNG 인코더 대신 훨씬 빠른 BmpBitmapEncoder를 사용합니다.
            var encoder = new BmpBitmapEncoder();
            encoder.Frames.Add(BitmapFrame.Create(source));
            using (var stream = new MemoryStream())
            {
                encoder.Save(stream);
                stream.Position = 0; // 스트림 위치를 처음으로 리셋

                var result = new BitmapImage();
                result.BeginInit();
                result.CacheOption = BitmapCacheOption.OnLoad;
                result.StreamSource = stream;
                result.EndInit();
                result.Freeze(); // UI 스레드 외에서도 접근 가능하도록 고정
                return result;
            }
        }

        private BitmapImage CloneBitmapImage(BitmapImage source)
        {
            return ConvertBitmapSourceToBitmapImage(source);
        }
    }
}