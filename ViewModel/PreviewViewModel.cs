using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Shapes;
using System.Windows.Controls.Primitives;

namespace ImageProcessing.Views.Controls
{
    public class PreviewControlViewModel
    {
        private readonly Image PreviewImageControl;
        private readonly Canvas PreviewCanvas;
        private readonly Rectangle ViewportRect;

        public event Action<Point>? ViewportDragged;

        public double ImageWidth { get; set; }
        public double ImageHeight { get; set; }
        public double ZoomLevel { get; set; } = 1.0;

        private DateTime lastUpdateTime = DateTime.MinValue;
        private const int UPDATE_THROTTLE_MS = 16;
        private bool isDragging = false;

        public PreviewControlViewModel(Image previewImageControl, Canvas previewCanvas, Rectangle viewportRect)
        {
            PreviewImageControl = previewImageControl;
            PreviewCanvas = previewCanvas;
            ViewportRect = viewportRect;
        }

        public void UpdateViewportRect(ScrollViewer scroll, double imageWidth, double imageHeight, double zoomLevel = 1.0)
        {
            var now = DateTime.Now;
            if ((now - lastUpdateTime).TotalMilliseconds < UPDATE_THROTTLE_MS)
                return;
            lastUpdateTime = now;

            if (PreviewImageControl.Source == null) return;

            ImageWidth = imageWidth;
            ImageHeight = imageHeight;
            ZoomLevel = zoomLevel;

            double previewWidth = PreviewImageControl.ActualWidth;
            double previewHeight = PreviewImageControl.ActualHeight;
            if (previewWidth == 0 || previewHeight == 0) return;

            PreviewCanvas.Width = previewWidth;
            PreviewCanvas.Height = previewHeight;

            double imageAspect = imageWidth / imageHeight;
            double previewAspect = previewWidth / previewHeight;

            double displayedWidth, displayedHeight;
            double offsetX = 0, offsetY = 0;

            if (imageAspect > previewAspect)
            {
                displayedWidth = previewWidth;
                displayedHeight = previewWidth / imageAspect;
                offsetY = (previewHeight - displayedHeight) / 2;
            }
            else
            {
                displayedHeight = previewHeight;
                displayedWidth = previewHeight * imageAspect;
                offsetX = (previewWidth - displayedWidth) / 2;
            }

            double actualImageWidth = scroll.ExtentWidth / zoomLevel;
            double actualImageHeight = scroll.ExtentHeight / zoomLevel;
            if (actualImageWidth == 0 || actualImageHeight == 0) return;

            double viewportRatioW = scroll.ViewportWidth / (actualImageWidth * zoomLevel);
            double viewportRatioH = scroll.ViewportHeight / (actualImageHeight * zoomLevel);
            double scrollRatioX = scroll.HorizontalOffset / (scroll.ExtentWidth - scroll.ViewportWidth + 0.001);
            double scrollRatioY = scroll.VerticalOffset / (scroll.ExtentHeight - scroll.ViewportHeight + 0.001);

            if (double.IsNaN(viewportRatioW) || double.IsNaN(viewportRatioH) ||
                double.IsNaN(scrollRatioX) || double.IsNaN(scrollRatioY))
                return;

            double rectW = displayedWidth * viewportRatioW;
            double rectH = displayedHeight * viewportRatioH;
            double rectX = offsetX + (displayedWidth - rectW) * scrollRatioX;
            double rectY = offsetY + (displayedHeight - rectH) * scrollRatioY;

            rectX = Math.Max(offsetX, Math.Min(rectX, offsetX + displayedWidth - rectW));
            rectY = Math.Max(offsetY, Math.Min(rectY, offsetY + displayedHeight - rectH));
            rectW = Math.Max(1, Math.Min(rectW, displayedWidth));
            rectH = Math.Max(1, Math.Min(rectH, displayedHeight));

            Canvas.SetLeft(ViewportRect, rectX);
            Canvas.SetTop(ViewportRect, rectY);
            ViewportRect.Width = rectW;
            ViewportRect.Height = rectH;
        }

        public void PreviewCanvas_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ChangedButton == MouseButton.Left)
            {
                var pos = e.GetPosition(PreviewCanvas);

                if (e.ClickCount == 2)
                {
                    // 더블클릭 → 확대/축소는 MainWindow에서 처리
                    RaiseViewportDragged(pos);
                }
                else
                {
                    // 단일 클릭 → 바로 이동
                    RaiseViewportDragged(pos);

                    // 드래그 시작
                    isDragging = true;
                    ((UIElement)sender).CaptureMouse();
                }
            }
        }


        public void PreviewCanvas_MouseMove(object sender, MouseEventArgs e)
        {
            // 드래그 중일 때만 위치를 업데이트
            if (isDragging && e.LeftButton == MouseButtonState.Pressed)
            {
                RaiseViewportDragged(e.GetPosition(PreviewCanvas));
            }
        }

        public void PreviewCanvas_MouseUp(object sender, MouseButtonEventArgs e)
        {
            if (e.ChangedButton == MouseButton.Left)
            {
                isDragging = false;
                ((UIElement)sender).ReleaseMouseCapture();
            }
        }
        private void RaiseViewportDragged(Point p)
        {
            double previewWidth = PreviewCanvas.ActualWidth;
            double previewHeight = PreviewCanvas.ActualHeight;
            if (previewWidth == 0 || previewHeight == 0 || ImageWidth == 0 || ImageHeight == 0)
                return;

            double imageAspect = ImageWidth / ImageHeight;
            double previewAspect = previewWidth / previewHeight;

            double displayedWidth, displayedHeight;
            double offsetX = 0, offsetY = 0;

            if (imageAspect > previewAspect)
            {
                displayedWidth = previewWidth;
                displayedHeight = previewWidth / imageAspect;
                offsetY = (previewHeight - displayedHeight) / 2;
            }
            else
            {
                displayedHeight = previewHeight;
                displayedWidth = previewHeight * imageAspect;
                offsetX = (previewWidth - displayedWidth) / 2;
            }

            double clampedX = Math.Max(offsetX, Math.Min(p.X, offsetX + displayedWidth));
            double clampedY = Math.Max(offsetY, Math.Min(p.Y, offsetY + displayedHeight));

            // 🔹 비율로 변환
            double ratioX = (clampedX - offsetX) / displayedWidth;
            double ratioY = (clampedY - offsetY) / displayedHeight;

            ViewportDragged?.Invoke(new Point(ratioX, ratioY));
        }

    }
}
