#include "pch.h"
#include "ImageProcessingWrapper.h"
#include "ImageProcessingEngineApp.h"
using namespace ImageProcessingWrapper;

void ImageEngine::ApplyGrayscale(array<System::Byte>^ pixels, int width, int height) {
    pin_ptr<unsigned char> p = &pixels[0];
    _nativeEngine->ApplyGrayscale(p, width, height);
}
//void ImageEngine::ApplyGaussianBlur(array<System::Byte>^ pixels, int width, int height, float sigma) {
//    pin_ptr<unsigned char> p = &pixels[0];
//    _nativeEngine->ApplyGaussianBlur(p, width, height, sigma);
//}
void ImageEngine::ApplyGaussianBlur(array<System::Byte>^ pixels, int width, int height, int radius) {
    pin_ptr<unsigned char> p = &pixels[0];
    _nativeEngine->ApplyGaussianBlur(p, width, height, radius);
}
void ImageEngine::ApplyMedian(array<System::Byte>^ pixels, int width, int height, int kernelSize) {
    pin_ptr<unsigned char> p = &pixels[0];
    _nativeEngine->ApplyMedian(p, width, height, kernelSize);
}

void ImageEngine::ApplyBinarization(array<System::Byte>^ pixels, int width, int height) {
    pin_ptr<unsigned char> p = &pixels[0];
    _nativeEngine->ApplyBinarization(p, width, height);
}

void ImageEngine::ApplyDilation(array<System::Byte>^ pixels, int width, int height) {
    pin_ptr<unsigned char> p = &pixels[0];
    _nativeEngine->ApplyDilation(p, width, height);
}

void ImageEngine::ApplyErosion(array<System::Byte>^ pixels, int width, int height) {
    pin_ptr<unsigned char> p = &pixels[0];
    _nativeEngine->ApplyErosion(p, width, height);
}

void ImageEngine::ApplySobel(array<System::Byte>^ pixels, int width, int height) {
    pin_ptr<unsigned char> p = &pixels[0];
    _nativeEngine->ApplySobel(p, width, height);
}

void ImageEngine::ApplyLaplacian(array<System::Byte>^ pixels, int width, int height) {
    pin_ptr<unsigned char> p = &pixels[0];
    _nativeEngine->ApplyLaplacian(p, width, height);
}

void ImageEngine::ApplyTemplateMatch(array<System::Byte>^ originalPixels, int width, int height, array<System::Byte>^ templatePixels, int templateWidth, int templateHeight, int% matchX, int% matchY){
    pin_ptr<unsigned char> p = &originalPixels[0];
    pin_ptr<unsigned char> t = &templatePixels[0];

    pin_ptr<int> px = &matchX;
    pin_ptr<int> py = &matchY;

    _nativeEngine->ApplyTemplateMatch(p, width, height, t,  templateWidth, templateHeight, px, py);
}

bool ImageEngine::ApplyFFT(array<System::Byte>^ pixels, int width, int height) {
    pin_ptr<unsigned char> p = &pixels[0];
    return _nativeEngine->ApplyFFT(p, width, height);
}

bool ImageEngine::ApplyIFFT(array<System::Byte>^ pixels, int width, int height) {
    pin_ptr<unsigned char> p = &pixels[0];
    return _nativeEngine->ApplyIFFT(p, width, height);
}

void ImageEngine::ClearFFTData() {
    _nativeEngine->ClearFFTData();
}
bool ImageEngine::HasFFTData() {
    return _nativeEngine->HasFFTData();
}