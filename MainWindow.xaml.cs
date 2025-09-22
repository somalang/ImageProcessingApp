using ImageProcessing.ViewModel;
using System.ComponentModel;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace ImageProcessingApp.Views
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        // --- 수정된 부분: 더블클릭 줌 상태를 관리하기 위한 변수 ---
        private bool _isDoubleClickZoomActive = false;
        private double _scaleBeforeDoubleClick;
        // ---------------------------------------------------------

        public MainWindow()
        {
            InitializeComponent();

            PreviewWindow.ViewportDragged += OnPreviewDragged;

            if (DataContext is MainViewModel vm)
            {
                vm.PropertyChanged += ViewModel_PropertyChanged;
            }
            ImageScrollViewer.ScrollChanged += ImageScrollViewer_ScrollChanged;
        }

        // --- 수정된 부분: 더블클릭 로직 전체 변경 ---
        private void Canvas_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ClickCount == 2)
            {
                if (DataContext is not MainViewModel vm) return;

                var scaleTransform = ImageScaleTransform;
                var position = e.GetPosition(DisplayImage);

                if (_isDoubleClickZoomActive)
                {
                    // 더블클릭 줌 해제 → 원래 배율로 복귀
                    vm.ZoomLevel = _scaleBeforeDoubleClick;
                    _isDoubleClickZoomActive = false;
                }
                else
                {
                    // 더블클릭 줌 시작 → 현재 배율 저장 후 더 확대
                    _scaleBeforeDoubleClick = vm.ZoomLevel;  // 🔹 현재 버튼 줌 포함한 상태 배율 저장
                    vm.ZoomLevel *= 1.5;  // 확대 (배율은 원하면 조정 가능)
                    _isDoubleClickZoomActive = true;
                }

                // 마우스 위치 기준으로 줌
                scaleTransform.CenterX = position.X;
                scaleTransform.CenterY = position.Y;

                // 스크롤 보정
                var scrollViewer = ImageScrollViewer;
                double newOffsetX = position.X * vm.ZoomLevel - scrollViewer.ViewportWidth / 2;
                double newOffsetY = position.Y * vm.ZoomLevel - scrollViewer.ViewportHeight / 2;
                scrollViewer.ScrollToHorizontalOffset(Math.Max(0, newOffsetX));
                scrollViewer.ScrollToVerticalOffset(Math.Max(0, newOffsetY));
            }
            else
            {
                // 단일 클릭: 영역 선택 시작
                if (DataContext is MainViewModel viewModel)
                {
                    var startPoint = e.GetPosition(sender as IInputElement);
                    viewModel.StartSelection(startPoint);
                }
            }
        }

        // ---------------------------------------------------------

        private void Canvas_MouseMove(object sender, MouseEventArgs e)
        {
            if (DataContext is MainViewModel viewModel)
            {
                var currentPoint = e.GetPosition(sender as IInputElement);
                viewModel.UpdateCoordinates(currentPoint);

                if (e.LeftButton == MouseButtonState.Pressed)
                {
                    viewModel.UpdateSelection(currentPoint);
                }
            }
        }

        private void Canvas_MouseUp(object sender, MouseButtonEventArgs e)
        {
            if (DataContext is MainViewModel viewModel)
            {
                viewModel.EndSelection();
            }
        }

        private void Canvas_MouseLeave(object sender, MouseEventArgs e)
        {
            if (DataContext is MainViewModel viewModel)
            {
                viewModel.ClearCoordinates();
            }
        }
        private void DisplayImage_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            if (DataContext is MainViewModel vm)
            {
                vm.ImageControlSize = new Size(DisplayImage.ActualWidth, DisplayImage.ActualHeight);
            }
            UpdatePreviewViewport();
        }

        private void ViewModel_PropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            if (e.PropertyName == nameof(MainViewModel.ZoomLevel))
            {
                var vm = DataContext as MainViewModel;
                if (vm != null)
                {
                    ImageScaleTransform.ScaleX = vm.ZoomLevel;
                    ImageScaleTransform.ScaleY = vm.ZoomLevel;
                }

                UpdatePreviewViewport();
            }
        }

        private void ImageScrollViewer_ScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            if (e.HorizontalChange != 0 || e.VerticalChange != 0)
            {
                UpdatePreviewViewport();
            }
        }
        private void OnPreviewDragged(Point ratio)
        {
            if (DataContext is not MainViewModel vm || vm.LoadedImage == null) return;

            double maxScrollX = Math.Max(0, ImageScrollViewer.ExtentWidth - ImageScrollViewer.ViewportWidth);
            double maxScrollY = Math.Max(0, ImageScrollViewer.ExtentHeight - ImageScrollViewer.ViewportHeight);

            // 클릭한 지점을 뷰포트 중심으로 이동
            double targetScrollX = ratio.X * ImageScrollViewer.ExtentWidth - ImageScrollViewer.ViewportWidth / 2;
            double targetScrollY = ratio.Y * ImageScrollViewer.ExtentHeight - ImageScrollViewer.ViewportHeight / 2;

            targetScrollX = Math.Max(0, Math.Min(targetScrollX, maxScrollX));
            targetScrollY = Math.Max(0, Math.Min(targetScrollY, maxScrollY));

            ImageScrollViewer.ScrollToHorizontalOffset(targetScrollX);
            ImageScrollViewer.ScrollToVerticalOffset(targetScrollY);
        }


        private DateTime lastPreviewUpdate = DateTime.MinValue;
        private const int PREVIEW_UPDATE_THROTTLE_MS = 16;

        private void UpdatePreviewViewport()
        {
            var now = DateTime.Now;
            if ((now - lastPreviewUpdate).TotalMilliseconds < PREVIEW_UPDATE_THROTTLE_MS)
                return;
            lastPreviewUpdate = now;

            if (DataContext is MainViewModel vm && vm.LoadedImage != null)
            {
                PreviewWindow.UpdateViewportRect(
                    ImageScrollViewer,
                    vm.LoadedImage.PixelWidth,
                    vm.LoadedImage.PixelHeight,
                    vm.ZoomLevel
                );
            }
        }

        private void ImageDisplayControl_Loaded(object sender, RoutedEventArgs e)
        {
            UpdatePreviewViewport();
        }
    }
}