using ImageProcessing.Models;
using ImageProcessing.Services;
using ImageProcessing.ViewModels;
using System;
using System.IO;
using System.Windows.Input;
using System.Windows;

namespace ImageProcessing.ViewModel
{
    public class SettingsViewModel : ViewModelBase
    {
        private readonly SettingService _settingService;
        private AppSettings _settings;

        public SettingsViewModel(SettingService settingService)
        {
            _settingService = settingService;
            LoadSettings();
            InitializeCommands();
        }

        #region Properties
        public AppSettings Settings
        {
            get => _settings;
            set => SetProperty(ref _settings, value);
        }

        // AppSettings 속성들
        public string DefaultImagePath
        {
            get => Settings.DefaultImagePath;
            set
            {
                if (Settings.DefaultImagePath != value)
                {
                    Settings.DefaultImagePath = value;
                    OnPropertyChanged();
                }
            }
        }

        public double DefaultGaussianSigma
        {
            get => Settings.DefaultGaussianSigma;
            set
            {
                if (Settings.DefaultGaussianSigma != value)
                {
                    Settings.DefaultGaussianSigma = value;
                    OnPropertyChanged();
                }
            }
        }

        public int DefaultLaplacianKernelType
        {
            get => Settings.DefaultLaplacianKernelType;
            set
            {
                if (Settings.DefaultLaplacianKernelType != value)
                {
                    Settings.DefaultLaplacianKernelType = value;
                    OnPropertyChanged();
                }
            }
        }

        public int DefaultBinarizationThreshold
        {
            get => Settings.DefaultBinarizationThreshold;
            set
            {
                if (Settings.DefaultBinarizationThreshold != value)
                {
                    Settings.DefaultBinarizationThreshold = value;
                    OnPropertyChanged();
                }
            }
        }

        public int DefaultDilationKernelSize
        {
            get => Settings.DefaultDilationKernelSize;
            set
            {
                if (Settings.DefaultDilationKernelSize != value)
                {
                    Settings.DefaultDilationKernelSize = value;
                    OnPropertyChanged();
                }
            }
        }

        public int DefaultErosionKernelSize
        {
            get => Settings.DefaultErosionKernelSize;
            set
            {
                if (Settings.DefaultErosionKernelSize != value)
                {
                    Settings.DefaultErosionKernelSize = value;
                    OnPropertyChanged();
                }
            }
        }

        public int DefaultMedianKernelSize
        {
            get => Settings.DefaultMedianKernelSize;
            set
            {
                if (Settings.DefaultMedianKernelSize != value)
                {
                    Settings.DefaultMedianKernelSize = value;
                    OnPropertyChanged();
                }
            }
        }
        #endregion

        #region Commands
        public ICommand SaveCommand { get; private set; }
        public ICommand CancelCommand { get; private set; }
        public ICommand ResetToDefaultCommand { get; private set; }
        public ICommand BrowseImagePathCommand { get; private set; }
        #endregion

        #region Events
        public event EventHandler SettingsSaved;
        public event EventHandler SettingsCancelled;
        #endregion

        private void InitializeCommands()
        {
            SaveCommand = new RelayCommand(_ => SaveSettings());
            CancelCommand = new RelayCommand(_ => Cancel());
            ResetToDefaultCommand = new RelayCommand(_ => ResetToDefault());
            BrowseImagePathCommand = new RelayCommand(_ => BrowseImagePath());
        }

        private void LoadSettings()
        {
            Settings = _settingService.LoadAppSettings();
        }

        private void SaveSettings()
        {
            // 1. 가우시안 블러 (Sigma) 값 유효성 검사
            if (DefaultGaussianSigma <= 0)
            {
                MessageBox.Show("가우시안 블러 (Sigma) 값은 0보다 커야 합니다.", "입력 오류", MessageBoxButton.OK, MessageBoxImage.Error);
                return; // 저장하지 않고 종료
            }

            // 2. 팽창 (커널 크기) 유효성 검사
            if (DefaultDilationKernelSize <= 0 || DefaultDilationKernelSize % 2 == 0)
            {
                MessageBox.Show("팽창 (커널 크기)는 0보다 큰 홀수여야 합니다.", "입력 오류", MessageBoxButton.OK, MessageBoxImage.Error);
                return; // 저장하지 않고 종료
            }

            // 3. 침식 (커널 크기) 유효성 검사
            if (DefaultErosionKernelSize <= 0 || DefaultErosionKernelSize % 2 == 0)
            {
                MessageBox.Show("침식 (커널 크기)은 0보다 큰 홀수여야 합니다.", "입력 오류", MessageBoxButton.OK, MessageBoxImage.Error);
                return; // 저장하지 않고 종료
            }

            // 4. 메디안 필터 (커널 크기) 유효성 검사
            if (DefaultMedianKernelSize <= 0 || DefaultMedianKernelSize % 2 == 0)
            {
                MessageBox.Show("메디안 필터 (커널 크기)는 0보다 큰 홀수여야 합니다.", "입력 오류", MessageBoxButton.OK, MessageBoxImage.Error);
                return; // 저장하지 않고 종료
            }
            try
            {
                _settingService.SaveSettings(Settings, null);
                SettingsSaved?.Invoke(this, EventArgs.Empty);
            }
            catch (Exception ex)
            {
                System.Windows.MessageBox.Show($"설정 저장 중 오류가 발생했습니다: {ex.Message}",
                    "오류", System.Windows.MessageBoxButton.OK, System.Windows.MessageBoxImage.Error);
            }
        }

        private void Cancel()
        {
            LoadSettings(); // 원래 설정으로 되돌리기
            SettingsCancelled?.Invoke(this, EventArgs.Empty);
        }

        private void ResetToDefault()
        {
            Settings = new AppSettings();
            // 모든 속성 변경 알림
            OnPropertyChanged(nameof(DefaultImagePath));
            OnPropertyChanged(nameof(DefaultGaussianSigma));
            OnPropertyChanged(nameof(DefaultLaplacianKernelType));
            OnPropertyChanged(nameof(DefaultBinarizationThreshold));
            OnPropertyChanged(nameof(DefaultDilationKernelSize));
            OnPropertyChanged(nameof(DefaultErosionKernelSize));
            OnPropertyChanged(nameof(DefaultMedianKernelSize));
        }

        private void BrowseImagePath()
        {
            var dialog = new Microsoft.Win32.OpenFileDialog
            {
                Title = "기본 이미지 경로 선택",
                Filter = "폴더 선택|*.*",
                CheckFileExists = false,
                CheckPathExists = true,
                FileName = "Folder Selection"
            };

            if (dialog.ShowDialog() == true)
            {
                DefaultImagePath = Path.GetDirectoryName(dialog.FileName);
            }
        }
    }
}
