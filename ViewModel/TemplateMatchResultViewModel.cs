using System;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media.Imaging;
using ImageProcessing.ViewModels;

namespace ImageProcessing.ViewModels
{
    public class TemplateMatchResultViewModel : ViewModelBase
    {
        private readonly Action<BitmapImage> _onApplyResult;
        private BitmapImage _resultImage;
        private string _matchResultText;
        private string _processingTime;

        public TemplateMatchResultViewModel(BitmapImage originalImage, BitmapImage resultImage,
            Rect matchedRect, long processingTimeMs, Action<BitmapImage> onApplyResult)
        {
            _onApplyResult = onApplyResult;
            OriginalImage = originalImage;
            ResultImage = resultImage;
            MatchedRect = matchedRect;
            ProcessingTime = $"{processingTimeMs} ms";

            // 매칭 결과 텍스트 생성
            if (!matchedRect.IsEmpty)
            {
                MatchResultText = $"매칭됨 - 위치: ({matchedRect.X:F0}, {matchedRect.Y:F0}), 크기: {matchedRect.Width:F0}×{matchedRect.Height:F0}";
            }
            else
            {
                MatchResultText = "매칭된 영역을 찾을 수 없음";
            }

            InitializeCommands();
        }

        #region Properties
        public BitmapImage OriginalImage { get; }

        public BitmapImage ResultImage
        {
            get => _resultImage;
            set => SetProperty(ref _resultImage, value);
        }

        public Rect MatchedRect { get; }

        public string MatchResultText
        {
            get => _matchResultText;
            set => SetProperty(ref _matchResultText, value);
        }

        public string ProcessingTime
        {
            get => _processingTime;
            set => SetProperty(ref _processingTime, value);
        }
        #endregion

        #region Commands
        public ICommand ApplyResultCommand { get; private set; }
        #endregion

        #region Events
        public event EventHandler ResultApplied;
        #endregion

        private void InitializeCommands()
        {
            ApplyResultCommand = new RelayCommand(_ => ApplyResult(), _ => !MatchedRect.IsEmpty);
        }

        private void ApplyResult()
        {
            if (ResultImage != null && _onApplyResult != null)
            {
                _onApplyResult(ResultImage);
                ResultApplied?.Invoke(this, EventArgs.Empty);
            }
        }
    }
}