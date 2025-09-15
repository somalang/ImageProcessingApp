using ImageProcessing.Services;
using ImageProcessing.ViewModels;
using System;
using System.IO;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media.Imaging;
using System.Diagnostics;
using ImageProcessing.Models;
using ImageProcessing.Views;
using System.Windows.Media;
using System.Linq;
using System.Windows.Shapes;
using ImageProcessing.Utilities;

namespace ImageProcessing.ViewModel
{
    public class MainViewModel : ViewModelBase
    {
        #region Constants
        private const double MIN_ZOOM_LEVEL = 0.5;
        private const double MAX_ZOOM_LEVEL = 3.0;
        private const double ZOOM_STEP = 0.1;
        private const int MIN_SELECTION_SIZE = 5;
        #endregion

        #region Fields
        // 서비스 필드
        //private readonly ImageProcessor imageProcessor;
        private readonly FileService fileService;
        private readonly SettingService settingService;
        private readonly LogService logService;
        private readonly ClipboardService clipboardService;

        // 이미지 관련 필드
        private BitmapImage currentBitmapImage;
        private BitmapImage originalImage;
        private BitmapImage loadedImage;
        private double baseScale;

        // 선택 영역 및 좌표 관련 필드
        private Visibility selectionVisibility;
        private Rect selectionRect;
        private bool isSelecting;
        private Point startPoint;
        private string currentCoordinates;

        // UI 및 상태 관련 필드
        private Size imageControlSize;
        private double zoomLevel = 1.0;
        private BitmapSource _previewImage;
        private readonly ImageProcessor _imageProcessor;
        private readonly GPUImageProcessor _gpuImageProcessor;

        // 기타
        private string lastImagePath;
        private string processingTime;

        // 윈도우 인스턴스
        private OriginalImageView originalImageView;
        private LogWindow logWindow;

        // 설정 관련 필드
        private AppSettings _appSettings;
        private FilterParameters _filterParameters;
        #endregion

        #region Properties
        // 이미지 관련 속성
        public BitmapImage CurrentBitmapImage
        {
            get => currentBitmapImage;
            set
            {
                if (SetProperty(ref currentBitmapImage, value))
                {
                    OnPropertyChanged(nameof(CanUndo));
                    OnPropertyChanged(nameof(CanRedo));
                    UpdatePreviewImage();
                }
            }
        }

        public BitmapImage LoadedImage
        {
            get => loadedImage;
            set
            {
                if (SetProperty(ref loadedImage, value))
                {
                    UpdatePreviewImage(); // 추가
                }
            }
        }

        // 선택 영역 및 좌표 속성
        public Visibility SelectionVisibility
        {
            get => selectionVisibility;
            set => SetProperty(ref selectionVisibility, value);
        }

        public Rect SelectionRect
        {
            get => selectionRect;
            set => SetProperty(ref selectionRect, value);
        }

        public string CurrentCoordinates
        {
            get => currentCoordinates;
            set => SetProperty(ref currentCoordinates, value);
        }

        // UI 및 상태 속성
        public Size ImageControlSize
        {
            get => imageControlSize;
            set
            {
                if (SetProperty(ref imageControlSize, value))
                {
                    // 이미지가 로드되어 있다면 baseScale 재계산
                    if (CurrentBitmapImage != null)
                        InitializeBaseScale(imageControlSize, CurrentBitmapImage);
                }
            }
        }

        public string ProcessingTime
        {
            get => processingTime;
            set => SetProperty(ref processingTime, value);
        }

        public double ZoomLevel
        {
            get => zoomLevel;
            set
            {
                if (SetProperty(ref zoomLevel, value))
                {
                    if (zoomLevel < MIN_ZOOM_LEVEL) zoomLevel = MIN_ZOOM_LEVEL;
                    if (zoomLevel > MAX_ZOOM_LEVEL) zoomLevel = MAX_ZOOM_LEVEL;
                    OnPropertyChanged(nameof(ZoomPercentage));
                }
            }
        }



        public string ZoomPercentage => $"{ZoomLevel * 100:0}%";
        private double? previousZoomLevelForDoubleClick = null;

        public BitmapSource PreviewImage
        {
            get => _previewImage;
            set
            {
                _previewImage = value;
                OnPropertyChanged(nameof(PreviewImage)); // UI에 변경사항 알림
                OnPropertyChanged(nameof(PreviewVisibility)); // 프리뷰 창의 보임/숨김 상태도 갱신
            }
        }
        public Visibility PreviewVisibility => PreviewImage != null ? Visibility.Visible : Visibility.Collapsed;

        // 설정 관련 속성
        public AppSettings AppSettings
        {
            get => _appSettings;
            set => SetProperty(ref _appSettings, value);
        }

        public FilterParameters FilterParameters
        {
            get => _filterParameters;
            set => SetProperty(ref _filterParameters, value);
        }

        // Undo/Redo 상태
        public bool CanUndo => _imageProcessor.CanUndo;
        public bool CanRedo => _imageProcessor.CanRedo;

        // Command 속성
        public ICommand LoadImageCommand { get; private set; }
        public ICommand SaveImageCommand { get; private set; }
        public ICommand ShowOriginalImageCommand { get; private set; }
        public ICommand DeleteImageCommand { get; private set; }
        public ICommand ReloadImageCommand { get; private set; }
        public ICommand ExitCommand { get; private set; }
        public ICommand UndoCommand { get; private set; }
        public ICommand RedoCommand { get; private set; }
        public ICommand CutSelectionCommand { get; private set; }
        public ICommand CopySelectionCommand { get; private set; }
        public ICommand PasteCommand { get; private set; }
        public ICommand DeleteSelectionCommand { get; private set; }
        public ICommand ApplyGrayscaleCommand { get; private set; }
        public ICommand ApplyGaussianBlurCommand { get; private set; }
        public ICommand ApplyMedianFilterCommand { get; private set; }
        public ICommand ApplyLaplacianCommand { get; private set; }
        public ICommand ApplySobelCommand { get; private set; }
        public ICommand ApplyBinarizationCommand { get; private set; }
        public ICommand ApplyDilationCommand { get; private set; }
        public ICommand ApplyErosionCommand { get; private set; }
        public ICommand FFTCommand { get; private set; }
        public ICommand IFFTCommand { get; private set; }
        public ICommand TemplateMatchCommand { get; private set; }
        public ICommand GenerateReportCommand { get; private set; }
        public ICommand OpenSettingsCommand { get; private set; }
        public ICommand ShowLogWindowCommand { get; private set; }
        public ICommand ZoomInCommand { get; private set; }
        public ICommand ZoomOutCommand { get; private set; }
        #endregion

        #region Constructor
        public MainViewModel()
        {
            //imageProcessor = new ImageProcessor();
            fileService = new FileService();
            settingService = new SettingService();
            logService = new LogService();
            clipboardService = new ClipboardService();
            _imageProcessor = new ImageProcessor();
            _gpuImageProcessor = new GPUImageProcessor();

            lastImagePath = settingService.GetLastImagePath();
            ProcessingTime = "Process Time: 0 ms";
            CurrentCoordinates = "좌표: X=0, Y=0";

            isSelecting = false;
            SelectionVisibility = Visibility.Collapsed;
            SelectionRect = new Rect(0, 0, 0, 0);

            InitializeCommands();
            LoadApplicationSettings(); // 설정 로드 추가

        }
        #endregion

        #region Public Methods
        public void UpdateCoordinates(Point point)
        {
            CurrentCoordinates = $"좌표: X={point.X:F0}, Y={point.Y:F0}";
        }

        public void ClearCoordinates()
        {
            CurrentCoordinates = "좌표: X=0, Y=0";
        }

        public void StartSelection(Point startPoint)
        {
            isSelecting = true;
            var imagePoint = ToImageCoordinates(startPoint, ImageControlSize, CurrentBitmapImage);
            var uiPoint = ConvertImageRectToUiRect(new Rect(imagePoint, new Size(0, 0)), ImageControlSize, CurrentBitmapImage).TopLeft;
            this.startPoint = uiPoint;
            SelectionRect = new Rect(uiPoint, new Size(0, 0));
            SelectionVisibility = Visibility.Visible;
        }

        public void UpdateSelection(Point currentPoint)
        {
            if (isSelecting)
            {
                var imagePoint = ToImageCoordinates(currentPoint, ImageControlSize, CurrentBitmapImage);
                var uiPoint = ConvertImageRectToUiRect(new Rect(imagePoint, new Size(0, 0)), ImageControlSize, CurrentBitmapImage).TopLeft;
                var x = Math.Min(startPoint.X, uiPoint.X);
                var y = Math.Min(startPoint.Y, uiPoint.Y);
                var width = Math.Abs(startPoint.X - uiPoint.X);
                var height = Math.Abs(startPoint.Y - uiPoint.Y);
                SelectionRect = new Rect(x, y, width, height);
            }
        }

        public void EndSelection()
        {
            isSelecting = false;
            if (SelectionRect.Width < MIN_SELECTION_SIZE || SelectionRect.Height < MIN_SELECTION_SIZE)
            {
                ResetSelection();
            }
        }
        #endregion

        #region Private Methods
        private void InitializeCommands()
        {
            LoadImageCommand = new RelayCommand(async _ => await LoadImageAsync());
            SaveImageCommand = new RelayCommand(async _ => await SaveImageAsync(), _ => CurrentBitmapImage != null);

            ApplyGrayscaleCommand = new RelayCommand(_ => ApplyFilter(() => {
                // GPU 처리를 위해 BitmapSource를 System.Drawing.Bitmap으로 변환
                var originalBitmap = ImageUtility.BitmapSourceToBitmap(CurrentBitmapImage);
                // GPU로 처리
                var processedBitmapSource = _gpuImageProcessor.ApplyGrayscaleFilter(originalBitmap);
                // 결과를 다시 BitmapImage로 변환하여 반환
                return BitmapSourceToBitmapImage(processedBitmapSource);
            }, "Grayscale (GPU)"));

            // 2. 나머지 필터들은 모두 기존의 CPU 방식(_imageProcessor)으로 되돌립니다.
            ApplySobelCommand = new RelayCommand(_ => ApplyFilter(() =>
                _imageProcessor.ApplySobel(CurrentBitmapImage), "Sobel"));

            ApplyLaplacianCommand = new RelayCommand(_ => ApplyFilter(() =>
                _imageProcessor.ApplyLaplacian(CurrentBitmapImage, FilterParameters.LaplacianKernelType), "Laplacian"));

            ApplyGaussianBlurCommand = new RelayCommand(_ => ApplyFilter(() =>
                _imageProcessor.ApplyGaussianBlur(CurrentBitmapImage, (float)FilterParameters.GaussianSigma), "Gaussian Blur"));

            ApplyBinarizationCommand = new RelayCommand(_ => ApplyFilter(() =>
                _imageProcessor.ApplyBinarization(CurrentBitmapImage, FilterParameters.BinarizationThreshold), "Binarization"));

            ApplyDilationCommand = new RelayCommand(_ => ApplyFilter(() =>
                _imageProcessor.ApplyDilation(CurrentBitmapImage, FilterParameters.DilationKernelSize), "Dilation"));

            ApplyErosionCommand = new RelayCommand(_ => ApplyFilter(() =>
                _imageProcessor.ApplyErosion(CurrentBitmapImage, FilterParameters.ErosionKernelSize), "Erosion"));

            ApplyMedianFilterCommand = new RelayCommand(_ => ApplyFilter(() =>
                _imageProcessor.ApplyMedianFilter(CurrentBitmapImage, FilterParameters.MedianKernelSize), "Median Filter"));

            FFTCommand = new RelayCommand(_ => ExecuteFFT(), _ => CurrentBitmapImage != null);
            IFFTCommand = new RelayCommand(_ => ApplyIFFT(), _ => CurrentBitmapImage != null && _imageProcessor.HasFFTData);

            UndoCommand = new RelayCommand(_ => ExecuteUndo(), _ => CanUndo);
            RedoCommand = new RelayCommand(_ => ExecuteRedo(), _ => CanRedo);
            ShowOriginalImageCommand = new RelayCommand(_ => ShowOriginalImage(), _ => originalImage != null);
            DeleteImageCommand = new RelayCommand(_ => DeleteImage(), _ => CurrentBitmapImage != null);
            ReloadImageCommand = new RelayCommand(async _ => await ReloadImageAsync(), _ => originalImage != null || !string.IsNullOrEmpty(lastImagePath));
            ExitCommand = new RelayCommand(_ => Application.Current.Shutdown());
            ShowLogWindowCommand = new RelayCommand(_ => ShowLogWindow());

            CutSelectionCommand = new RelayCommand(_ => CutSelection(), _ => HasValidSelection());
            CopySelectionCommand = new RelayCommand(_ => CopySelection(), _ => HasValidSelection());
            DeleteSelectionCommand = new RelayCommand(_ => DeleteSelection(), _ => HasValidSelection());
            PasteCommand = new RelayCommand(_ => ExecutePaste(), _ => CurrentBitmapImage != null && clipboardService.GetImage() != null);
            TemplateMatchCommand = new RelayCommand(async _ => await ExecuteTemplateMatchAsync(), _ => CurrentBitmapImage != null);
            OpenSettingsCommand = new RelayCommand(_ => ShowSettingsWindow());
            GenerateReportCommand = new RelayCommand(_ => ShowAnalysisReport(), _ => CurrentBitmapImage != null);

            ZoomInCommand = new RelayCommand(_ => ZoomLevel += ZOOM_STEP);
            ZoomOutCommand = new RelayCommand(_ => ZoomLevel -= ZOOM_STEP);
        }

        private void InitializeBaseScale(Size controlSize, BitmapSource originalImage)
        {
            if (originalImage == null || controlSize.Width == 0 || controlSize.Height == 0)
            {
                baseScale = 1.0;
                return;
            }

            double controlWidth = controlSize.Width;
            double controlHeight = controlSize.Height;
            double imageWidth = originalImage.PixelWidth;
            double imageHeight = originalImage.PixelHeight;

            baseScale = Math.Min(controlWidth / imageWidth, controlHeight / imageHeight);
        }

        private bool HasValidSelection()
        {
            return CurrentBitmapImage != null &&
                   SelectionVisibility == Visibility.Visible &&
                   SelectionRect.Width > MIN_SELECTION_SIZE &&
                   SelectionRect.Height > MIN_SELECTION_SIZE;
        }

        private async Task LoadImageAsync()
        {
            var filePath = fileService.OpenImageFileDialog();
            if (!string.IsNullOrEmpty(filePath))
            {
                await LoadImageFromPathAsync(filePath);
            }
        }

        private async Task LoadImageFromPathAsync(string filePath)
        {
            LoadedImage = await fileService.LoadImage(filePath);
            originalImage = LoadedImage;
            CurrentBitmapImage = LoadedImage;
            lastImagePath = filePath;
            settingService.SaveLastImagePath(filePath);

            // 추가: baseScale 초기화
            InitializeBaseScale(ImageControlSize, CurrentBitmapImage);
            ZoomLevel = 1.0;
        }


        private void ShowOriginalImage()
        {
            if (originalImageView == null)
            {
                originalImageView = new OriginalImageView(originalImage);
                originalImageView.Owner = Application.Current.MainWindow;
                originalImageView.Closed += (sender, eventArgs) => originalImageView = null;
                originalImageView.Show();
            }
            else
            {
                originalImageView.Activate();
            }
        }

        private void DeleteImage()
        {
            originalImageView?.Close();
            CurrentBitmapImage = null;
            LoadedImage = null;
            originalImage = null;
            ClearCoordinates();
            ResetSelection();
            ZoomLevel = 1.0;
        }

        private async Task ReloadImageAsync()
        {
            if (!string.IsNullOrEmpty(lastImagePath) && File.Exists(lastImagePath))
            {
                await LoadImageFromPathAsync(lastImagePath);
            }
        }

        private async Task SaveImageAsync()
        {
            var filePath = fileService.SaveImageFileDialog();
            if (filePath != null && CurrentBitmapImage != null)
            {
                await fileService.SaveImage(CurrentBitmapImage, filePath);
            }
        }

        private void CutSelection()
        {
            if (!HasValidSelection()) return;
            var imageSelectionRect = ConvertUiRectToImageRect(SelectionRect, ImageControlSize, CurrentBitmapImage);
            if (imageSelectionRect.IsEmpty) return;

            var croppedImage = _imageProcessor.Crop(CurrentBitmapImage, imageSelectionRect);
            if (croppedImage != null)
            {
                clipboardService.SetImage(croppedImage);
                // 수정된 메서드 호출
                CurrentBitmapImage = _imageProcessor.ClearSelection(CurrentBitmapImage, imageSelectionRect);
                LoadedImage = CurrentBitmapImage;
                ResetSelection();
                logService.AddLog("Cut Selection", 0);

                // UI 갱신을 위해 필수!
                OnPropertyChanged(nameof(CanUndo));
                OnPropertyChanged(nameof(CanRedo));
            }
        }

        private void CopySelection()
        {
            if (!HasValidSelection()) return;

            var imageSelectionRect = ConvertUiRectToImageRect(SelectionRect, ImageControlSize, CurrentBitmapImage);
            if (imageSelectionRect.IsEmpty) return;

            var croppedImage = _imageProcessor.Crop(CurrentBitmapImage, imageSelectionRect);
            if (croppedImage != null)
            {
                clipboardService.SetImage(croppedImage);
                logService.AddLog("Copy Selection", 0);
            }
        }

        private void ExecutePaste()
        {
            var clipboardImage = clipboardService.GetImage();
            if (CurrentBitmapImage == null || clipboardImage == null) return;

            Point pasteLocation = HasValidSelection()
                ? ToImageCoordinates(SelectionRect.TopLeft, ImageControlSize, CurrentBitmapImage)
                : new Point(0, 0);

            var stopwatch = Stopwatch.StartNew();
            var pastedImageSource = _imageProcessor.Paste(CurrentBitmapImage, clipboardImage, pasteLocation);
            stopwatch.Stop();

            CurrentBitmapImage = BitmapSourceToBitmapImage(pastedImageSource);
            LoadedImage = CurrentBitmapImage;

            var pastedImageRect = new Rect(pasteLocation.X, pasteLocation.Y, clipboardImage.PixelWidth, clipboardImage.PixelHeight);
            SelectionRect = ConvertImageRectToUiRect(pastedImageRect, ImageControlSize, CurrentBitmapImage);
            SelectionVisibility = Visibility.Visible;

            ProcessingTime = $"Process Time: {stopwatch.ElapsedMilliseconds} ms";
            logService.AddLog("Paste", stopwatch.ElapsedMilliseconds);

            OnPropertyChanged(nameof(CanUndo));
            OnPropertyChanged(nameof(CanRedo));
        }

        private void DeleteSelection()
        {
            if (!HasValidSelection()) return;

            var imageSelectionRect = ConvertUiRectToImageRect(SelectionRect, ImageControlSize, CurrentBitmapImage);
            if (imageSelectionRect.IsEmpty) return;

            var clearedImage = _imageProcessor.ClearSelection(CurrentBitmapImage, imageSelectionRect);
            CurrentBitmapImage = BitmapSourceToBitmapImage(clearedImage);
            LoadedImage = CurrentBitmapImage;
            ResetSelection();
            logService.AddLog("Delete Selection", 0);

            OnPropertyChanged(nameof(CanUndo));
            OnPropertyChanged(nameof(CanRedo));
        }
        private void UpdatePreviewImage()
        {
            if (CurrentBitmapImage != null)
            {
                PreviewImage = CurrentBitmapImage;
            }
            else if (LoadedImage != null)
            {
                PreviewImage = LoadedImage;
            }
            else
            {
                PreviewImage = null;
            }
        }

        private Point ToImageCoordinates(Point controlPoint, Size controlSize, BitmapSource imageSource)
        {
            if (imageSource == null || controlSize.Width == 0 || controlSize.Height == 0) return new Point(0, 0);

            double imageWidth = imageSource.PixelWidth;
            double imageHeight = imageSource.PixelHeight;

            // baseScale 기반으로 effective scale 계산 (원본 크기에 맞춰 fit 한 뒤 Zoom 적용)
            double effectiveScale = baseScale;

            double xOffset = (controlSize.Width - imageWidth * effectiveScale) / 2;
            double yOffset = (controlSize.Height - imageHeight * effectiveScale) / 2;

            double x = (controlPoint.X - xOffset) / effectiveScale;
            double y = (controlPoint.Y - yOffset) / effectiveScale;

            x = Math.Max(0, Math.Min(x, imageWidth));
            y = Math.Max(0, Math.Min(y, imageHeight));

            return new Point(x, y);
        }

        private Rect ConvertUiRectToImageRect(Rect uiRect, Size controlSize, BitmapSource imageSource)
        {
            if (imageSource == null) return Rect.Empty;
            Point topLeft = ToImageCoordinates(uiRect.TopLeft, controlSize, imageSource);
            Point bottomRight = ToImageCoordinates(uiRect.BottomRight, controlSize, imageSource);
            return new Rect(topLeft, bottomRight);
        }

        private Rect ConvertImageRectToUiRect(Rect imageRect, Size controlSize, BitmapSource imageSource)
        {
            if (imageSource == null || controlSize.Width == 0 || controlSize.Height == 0) return Rect.Empty;

            double imageWidth = imageSource.PixelWidth;
            double imageHeight = imageSource.PixelHeight;

            double effectiveScale = baseScale;
            if (effectiveScale == 0) return Rect.Empty;

            double xOffset = (controlSize.Width - imageWidth * effectiveScale) / 2;
            double yOffset = (controlSize.Height - imageHeight * effectiveScale) / 2;

            double uiX = imageRect.X * effectiveScale + xOffset;
            double uiY = imageRect.Y * effectiveScale + yOffset;
            double uiWidth = imageRect.Width * effectiveScale;
            double uiHeight = imageRect.Height * effectiveScale;

            return new Rect(uiX, uiY, uiWidth, uiHeight);
        }


        private void ResetSelection()
        {
            SelectionVisibility = Visibility.Collapsed;
            SelectionRect = new Rect(0, 0, 0, 0);
        }

        //private void ExecuteWithParameter<T>(string operationName, Func<ImageProcessor, T, BitmapImage> filterAction, string defaultValue)
        //{
        //    if (CurrentBitmapImage == null) return;

        //    var dialog = new ParameterInputDialog($"{operationName} Parameter", "값을 입력하세요:", defaultValue)
        //    {
        //        Owner = Application.Current.MainWindow
        //    };

        //    if (dialog.ShowDialog() == true)
        //    {
        //        try
        //        {
        //            var parameter = (T)Convert.ChangeType(dialog.InputValue, typeof(T));
        //            ApplyFilter(() => filterAction(imageProcessor, parameter), operationName);
        //        }
        //        catch (FormatException)
        //        {
        //            MessageBox.Show("잘못된 형식의 값입니다.", "입력 오류", MessageBoxButton.OK, MessageBoxImage.Warning);
        //        }
        //        catch (Exception ex)
        //        {
        //            MessageBox.Show($"오류가 발생했습니다: {ex.Message}", "오류", MessageBoxButton.OK, MessageBoxImage.Error);
        //        }
        //    }
        //}


        //private void ApplyFilter(Func<BitmapImage> filterAction, string operationName)
        //{
        //    if (CurrentBitmapImage == null) return;

        //    var stopwatch = Stopwatch.StartNew();
        //    var newImage = filterAction();
        //    stopwatch.Stop();

        //    if (newImage != null)
        //    {
        //        CurrentBitmapImage = newImage;
        //        LoadedImage = CurrentBitmapImage;
        //        ProcessingTime = $"Process Time: {stopwatch.ElapsedMilliseconds} ms";
        //        logService.AddLog(operationName, stopwatch.ElapsedMilliseconds);
        //    }
        //}

        //private void ApplyFilter(Func<BitmapImage> filterAction, string operationName)
        //{
        //    if (CurrentBitmapImage == null) return;

        //    var stopwatch = Stopwatch.StartNew();

        //    try
        //    {
        //        var processedImage = filterAction();
        //        if (processedImage != null)
        //        {
        //            CurrentBitmapImage = processedImage;
        //            LoadedImage = processedImage;

        //            stopwatch.Stop();
        //            ProcessingTime = $"Process Time: {stopwatch.ElapsedMilliseconds} ms";
        //            logService.AddLog(operationName, stopwatch.ElapsedMilliseconds);
        //        }
        //    }
        //    catch (Exception ex)
        //    {
        //        MessageBox.Show($"[{operationName}] 처리 중 오류: {ex.Message}",
        //            "오류", MessageBoxButton.OK, MessageBoxImage.Error);
        //    }
        //}
        // ViewModel/MainViewModel.cs
        private void ApplyFilter(Func<BitmapImage> filterAction, string operationName)
        {
            if (CurrentBitmapImage == null) return;

            var stopwatch = Stopwatch.StartNew();

            try
            {
                var processedImage = filterAction();
                if (processedImage != null)
                {
                    CurrentBitmapImage = processedImage;
                    LoadedImage = processedImage; // LoadedImage도 함께 갱신

                    stopwatch.Stop();
                    ProcessingTime = $"Process Time: {stopwatch.ElapsedMilliseconds} ms";
                    logService.AddLog(operationName, stopwatch.ElapsedMilliseconds);
                }
            }
            catch (Exception ex)
            {
                stopwatch.Stop(); // 오류 발생 시에도 시간 측정 중지
                MessageBox.Show($"[{operationName}] 처리 중 오류: {ex.Message}",
                                "오류", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }



        private void ApplyIFFT()
        {
            if (!_imageProcessor.HasFFTData)
            {
                MessageBox.Show("FFT 데이터가 없습니다. 먼저 푸리에 변환을 수행해주세요.", "경고", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            ApplyFilter(() => _imageProcessor.ApplyIFFT(CurrentBitmapImage), "IFFT");
            _imageProcessor.ClearFFTData();
        }

        private void ExecuteUndo()
        {
            CurrentBitmapImage = _imageProcessor.Undo(CurrentBitmapImage);
            LoadedImage = CurrentBitmapImage;

            OnPropertyChanged(nameof(CanUndo));
            OnPropertyChanged(nameof(CanRedo));
        }

        private void ExecuteRedo()
        {
            CurrentBitmapImage = _imageProcessor.Redo(CurrentBitmapImage);
            LoadedImage = CurrentBitmapImage;

            OnPropertyChanged(nameof(CanUndo));
            OnPropertyChanged(nameof(CanRedo));
        }


        private void ExecuteFFT()
        {
            if (CurrentBitmapImage == null) return;

            var stopwatch = Stopwatch.StartNew();

            try
            {
                // ApplyFFT가 새로운 BitmapImage를 반환하므로 이를 받아서 CurrentBitmapImage에 할당
                var fftResult = _imageProcessor.ApplyFFT(CurrentBitmapImage);

                stopwatch.Stop();

                // FFT 결과가 성공적으로 생성되었는지 확인
                if (fftResult != null && _imageProcessor.HasFFTData)
                {
                    // 새로운 FFT 결과 이미지로 업데이트
                    CurrentBitmapImage = fftResult;
                    LoadedImage = fftResult;

                    ProcessingTime = $"Process Time: {stopwatch.ElapsedMilliseconds} ms";
                    logService.AddLog("FFT", stopwatch.ElapsedMilliseconds);

                    CommandManager.InvalidateRequerySuggested();
                }
                else
                {
                    MessageBox.Show("푸리에 변환에 실패했습니다.", "오류",
                                   MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }
            catch (Exception ex)
            {
                stopwatch.Stop();
                MessageBox.Show($"푸리에 변환 중 오류가 발생했습니다: {ex.Message}", "오류",
                               MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }
        private async Task ExecuteTemplateMatchAsync()
        {
            if (CurrentBitmapImage == null) return;

            var templatePath = fileService.OpenImageFileDialog();
            if (string.IsNullOrEmpty(templatePath)) return;

            BitmapSource templateImage = await fileService.LoadImage(templatePath);
            if (templateImage == null) return;

            var stopwatch = Stopwatch.StartNew();
            Rect matchedRect = _imageProcessor.TemplateMatch(CurrentBitmapImage, templateImage);
            stopwatch.Stop();

            BitmapImage resultImage = null;

            if (!matchedRect.IsEmpty)
            {
                var sourceBitmap = new FormatConvertedBitmap(CurrentBitmapImage, PixelFormats.Pbgra32, null, 0);
                var drawingVisual = new DrawingVisual();

                using (DrawingContext drawingContext = drawingVisual.RenderOpen())
                {
                    drawingContext.DrawImage(sourceBitmap, new Rect(0, 0, sourceBitmap.PixelWidth, sourceBitmap.PixelHeight));

                    // 매칭된 영역에 빨간색 사각형 그리기 (더 두껍게)
                    var redPen = new Pen(Brushes.Red, 4);
                    drawingContext.DrawRectangle(Brushes.Transparent, redPen, matchedRect);

                    // 추가로 매칭 영역을 반투명 빨간색으로 채우기
                    var redBrush = new SolidColorBrush(Color.FromArgb(80, 255, 0, 0));
                    drawingContext.DrawRectangle(redBrush, redPen, matchedRect);
                }

                var renderTarget = new RenderTargetBitmap(sourceBitmap.PixelWidth, sourceBitmap.PixelHeight, 96, 96, PixelFormats.Pbgra32);
                renderTarget.Render(drawingVisual);

                resultImage = BitmapSourceToBitmapImage(renderTarget);
            }
            else
            {
                // 매칭되지 않은 경우 원본 이미지 사용
                resultImage = CurrentBitmapImage;
            }

            ShowTemplateMatchResult(CurrentBitmapImage, resultImage, matchedRect, stopwatch.ElapsedMilliseconds);

            ProcessingTime = $"Process Time: {stopwatch.ElapsedMilliseconds} ms";
            logService.AddLog("Template Matching", stopwatch.ElapsedMilliseconds);
        }

        private void ShowTemplateMatchResult(BitmapImage originalImage, BitmapImage resultImage,
            Rect matchedRect, long processingTime)
        {
            var resultWindow = new TemplateMatchResultWindow(
                originalImage,
                resultImage,
                matchedRect,
                processingTime,
                (appliedImage) =>
                {
                    // 결과 적용 콜백
                    CurrentBitmapImage = appliedImage;
                    LoadedImage = appliedImage;

                    if (!matchedRect.IsEmpty)
                    {
                        // 매칭된 영역을 선택 영역으로 설정
                        SelectionRect = ConvertImageRectToUiRect(matchedRect, ImageControlSize, CurrentBitmapImage);
                        SelectionVisibility = Visibility.Visible;
                    }
                })
            {
                Owner = Application.Current.MainWindow
            };

            resultWindow.Show();
        }
        private void ShowLogWindow()
        {
            if (logWindow == null)
            {
                logWindow = new LogWindow(logService)
                {
                    Owner = Application.Current.MainWindow
                };
                logWindow.Closed += (sender, eventArgs) => logWindow = null;
                logWindow.Show();
            }
            else
            {
                logWindow.Activate();
            }
        }
        private void ShowSettingsWindow()
        {
            // settingsWindow를 생성할 때 settingService를 인자로 전달합니다.
            var settingsWindow = new SettingsWindow(settingService)
            {
                Owner = Application.Current.MainWindow
            };

            if (settingsWindow.ShowDialog() == true)
            {
                // 설정이 저장된 후 다시 로드
                LoadApplicationSettings();

                // 설정 변경 알림 (필요시)
                ProcessingTime = "설정이 업데이트되었습니다.";
            }
        }
        private void LoadApplicationSettings()
        {
            AppSettings = settingService.LoadAppSettings();
            FilterParameters = settingService.LoadFilterParameters();
        }

        private void ShowAnalysisReport()
        {
            if (CurrentBitmapImage == null) return;

            // 1. 분석 서비스 생성 및 보고서 생성
            var analysisService = new AnalysisService();
            var report = analysisService.GenerateReport(CurrentBitmapImage, logService.GetLogs());

            // 2. 보고서 창 표시
            var reportWindow = new AnalysisReportWindow(report)
            {
                Owner = Application.Current.MainWindow
            };
            reportWindow.Show();
        }

        private BitmapImage BitmapSourceToBitmapImage(BitmapSource source)
        {
            if (source == null)
                return null;

            if (source is BitmapImage bitmapImage && bitmapImage.StreamSource == null)
            {
                return bitmapImage;
            }

            var encoder = new PngBitmapEncoder();
            encoder.Frames.Add(BitmapFrame.Create(source));

            using (var stream = new MemoryStream())
            {
                encoder.Save(stream);
                stream.Seek(0, SeekOrigin.Begin);

                var result = new BitmapImage();
                result.BeginInit();
                result.CacheOption = BitmapCacheOption.OnLoad;
                result.StreamSource = stream;
                result.EndInit();
                result.Freeze();

                return result;
            }
        }
        #endregion
    }
}