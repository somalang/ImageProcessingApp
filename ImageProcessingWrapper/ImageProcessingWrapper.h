#pragma once
#include "ImageProcessingEngineApp.h"

using namespace System;

namespace ImageProcessingWrapper {

    public ref class ImageEngine
    {
    private:
        NativeEngine::ImageProcessingEngine* _nativeEngine;


    public:
        ImageEngine() {
            _nativeEngine = new NativeEngine::ImageProcessingEngine();
        }

        ~ImageEngine() { this->!ImageEngine(); }
        !ImageEngine() { delete _nativeEngine; }

        void ApplyGrayscale(array<System::Byte>^ pixels, int width, int height);
        //void ApplyGaussianBlur(array<Byte>^ data, int width, int height, float sigma);
        void ApplyGaussianBlur(array<Byte>^ data, int width, int height, int radius);

        void ApplyMedian(array<System::Byte>^ pixels, int width, int height, int kernelSize);
        void ApplyBinarization(array<System::Byte>^ pixels, int width, int height);
        void ApplyDilation(array<System::Byte>^ pixels, int width, int height);
        void ApplyErosion(array<System::Byte>^ pixels, int width, int height);
        void ApplySobel(array<System::Byte>^ pixels, int width, int height);
        void ApplyLaplacian(array<System::Byte>^ pixels, int width, int height);
        void ApplyTemplateMatch(array<System::Byte>^ originalPixels, int width, int height, array<System::Byte>^ templatePixels, int templateWidth, int templateHeight, int% matchX, int% matchY);
        bool ApplyFFT(array<System::Byte>^ pixels, int width, int height);
        bool ApplyIFFT(array<System::Byte>^ pixels, int width, int height);
        void ClearFFTData();
        bool HasFFTData();
    };
}
