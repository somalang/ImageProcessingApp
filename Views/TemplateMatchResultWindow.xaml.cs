using System;
using System.Windows;
using System.Windows.Media.Imaging;
using ImageProcessing.ViewModels;

namespace ImageProcessing.Views
{
    public partial class TemplateMatchResultWindow : Window
    {
        public TemplateMatchResultViewModel ViewModel { get; private set; }

        public TemplateMatchResultWindow(BitmapImage originalImage, BitmapImage resultImage,
            System.Windows.Rect matchedRect, long processingTime, Action<BitmapImage> onApplyResult)
        {
            InitializeComponent();

            ViewModel = new TemplateMatchResultViewModel(originalImage, resultImage,
                matchedRect, processingTime, onApplyResult);
            DataContext = ViewModel;

            // 결과 적용 이벤트 구독
            ViewModel.ResultApplied += OnResultApplied;
        }

        private void OnResultApplied(object sender, EventArgs e)
        {
            Close();
        }

        private void CloseButton_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }

        protected override void OnClosed(EventArgs e)
        {
            // 이벤트 구독 해제
            if (ViewModel != null)
            {
                ViewModel.ResultApplied -= OnResultApplied;
            }
            base.OnClosed(e);
        }
    }
}