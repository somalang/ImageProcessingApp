using ImageProcessing.Services;
using ImageProcessing.ViewModel;
using System;
using System.Windows;

namespace ImageProcessing.Views
{
    public partial class SettingsWindow : Window
    {
        public SettingsViewModel ViewModel { get; private set; }

        public SettingsWindow(SettingService settingService)
        {
            InitializeComponent();

            ViewModel = new SettingsViewModel(settingService);
            DataContext = ViewModel;

            // 이벤트 구독
            ViewModel.SettingsSaved += OnSettingsSaved;
            ViewModel.SettingsCancelled += OnSettingsCancelled;
        }

        private void OnSettingsSaved(object sender, EventArgs e)
        {
            MessageBox.Show("설정이 성공적으로 저장되었습니다.", "알림",
                MessageBoxButton.OK, MessageBoxImage.Information);
            DialogResult = true;
            Close();
        }

        private void OnSettingsCancelled(object sender, EventArgs e)
        {
            DialogResult = false;
            Close();
        }

        protected override void OnClosed(EventArgs e)
        {
            // 이벤트 구독 해제
            if (ViewModel != null)
            {
                ViewModel.SettingsSaved -= OnSettingsSaved;
                ViewModel.SettingsCancelled -= OnSettingsCancelled;
            }
            base.OnClosed(e);
        }
    }
}