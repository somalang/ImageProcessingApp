#pragma once

using namespace System;

namespace ImageProcessingWrapper {

    public ref class ImageProcessor
    {
    private:
        // 네이티브 엔진 객체
        ImageProcessingEngine* engine;

    public:
        ImageProcessor();
        ~ImageProcessor();
        !ImageProcessor();

        void ApplyGrayscale(array<System::Byte>^ pixels, int width, int height);
        void ApplyGaussianBlur(array<System::Byte>^ pixels, int width, int height, int sigma);
        void ApplyMedianFilter(array<System::Byte>^ pixels, int width, int height, int kernelSize);
        void ApplyBinarization(array<System::Byte>^ pixels, int width, int height);
        void ApplyDilation(array<System::Byte>^ pixels, int width, int height);
        void ApplyErosion(array<System::Byte>^ pixels, int width, int height);
        void ApplySobel(array<System::Byte>^ pixels, int width, int height);
        void ApplyLaplacian(array<System::Byte>^ pixels, int width, int height);
    };
}
