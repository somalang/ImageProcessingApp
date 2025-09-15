using ImageProcessing.Models;
using System;
using System.IO;
using System.Text.Json;

namespace ImageProcessing.Services
{
    public class SettingService
    {
        private readonly string _settingsFilePath;
        private readonly string _appSettingsFilePath;

        public SettingService()
        {
            string appDataPath = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData);
            string appFolderPath = Path.Combine(appDataPath, "ImageProcessingApp");
            Directory.CreateDirectory(appFolderPath);

            _settingsFilePath = Path.Combine(appFolderPath, "settings.txt");
            _appSettingsFilePath = Path.Combine(appFolderPath, "app_settings.json");
        }

        #region Existing Methods
        public void SaveLastImagePath(string path)
        {
            File.WriteAllText(_settingsFilePath, path ?? string.Empty);
        }

        public string GetLastImagePath()
        {
            if (File.Exists(_settingsFilePath))
            {
                return File.ReadAllText(_settingsFilePath);
            }
            return null;
        }
        #endregion

        #region New Setting Methods
        public AppSettings LoadAppSettings()
        {
            try
            {
                if (File.Exists(_appSettingsFilePath))
                {
                    var json = File.ReadAllText(_appSettingsFilePath);
                    var settingsData = JsonSerializer.Deserialize<SettingsData>(json);
                    return settingsData?.AppSettings ?? new AppSettings();
                }
            }
            catch (Exception)
            {
                // Use default on failure
            }
            return new AppSettings();
        }

        public FilterParameters LoadFilterParameters()
        {
            var appSettings = LoadAppSettings();
            // AppSettings의 모든 기본값을 FilterParameters의 각 속성에 매핑합니다.
            return new FilterParameters
            {
                GaussianSigma = appSettings.DefaultGaussianSigma,
                LaplacianKernelType = appSettings.DefaultLaplacianKernelType,
                BinarizationThreshold = appSettings.DefaultBinarizationThreshold,
                DilationKernelSize = appSettings.DefaultDilationKernelSize,
                ErosionKernelSize = appSettings.DefaultErosionKernelSize,
                MedianKernelSize = appSettings.DefaultMedianKernelSize
            };
        }

        public void SaveSettings(AppSettings appSettings, FilterParameters filterParameters)
        {
            try
            {
                var settingsData = new SettingsData
                {
                    AppSettings = appSettings,
                    // We don't save runtime FilterParameters, only AppSettings which define the defaults.
                    FilterParameters = null
                };

                var json = JsonSerializer.Serialize(settingsData, new JsonSerializerOptions
                {
                    WriteIndented = true
                });

                File.WriteAllText(_appSettingsFilePath, json);
            }
            catch (Exception ex)
            {
                throw new InvalidOperationException($"Error saving settings: {ex.Message}", ex);
            }
        }

        public string GetSettingsFilePath()
        {
            return _appSettingsFilePath;
        }
        #endregion

        private class SettingsData
        {
            public AppSettings AppSettings { get; set; }
            public FilterParameters FilterParameters { get; set; }
        }
    }
}