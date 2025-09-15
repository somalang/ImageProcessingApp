using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace ImageProcessing.Views.Controls
{
    public partial class PreviewControl : UserControl
    {
        private readonly PreviewControlViewModel _viewModel;

        public PreviewControl()
        {
            InitializeComponent();
            _viewModel = new PreviewControlViewModel(PreviewImageControl, PreviewCanvas, ViewportRect);
        }

        public event Action<Point>? ViewportDragged
        {
            add => _viewModel.ViewportDragged += value;
            remove => _viewModel.ViewportDragged -= value;
        }

        public void UpdateViewportRect(ScrollViewer scroll, double imageWidth, double imageHeight, double zoomLevel = 1.0)
        {
            _viewModel.UpdateViewportRect(scroll, imageWidth, imageHeight, zoomLevel);
        }

        private void PreviewCanvas_MouseDown(object sender, MouseButtonEventArgs e)
        {
            _viewModel.PreviewCanvas_MouseDown(sender, e);
        }

        private void PreviewCanvas_MouseMove(object sender, MouseEventArgs e)
        {
            _viewModel.PreviewCanvas_MouseMove(sender, e);
        }

        private void PreviewCanvas_MouseUp(object sender, MouseButtonEventArgs e)
        {
            _viewModel.PreviewCanvas_MouseUp(sender, e);
        }
    }
}
