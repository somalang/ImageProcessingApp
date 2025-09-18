#include "ImageProcessingWrapper.h"
#include "ImageProcessingEngineApp.h"

using namespace ImageProcessingWrapper;

ImageProcessor::ImageProcessor() {
    engine = new ImageProcessingEngine();
}

ImageProcessor::~ImageProcessor() {
    this->!ImageProcessor();
}

ImageProcessor::!ImageProcessor() {
    if (engine != nullptr) {
        delete engine;
        engine = nullptr;
    }
}

void ImageProcessor::ApplyGrayscale(array<System::Byte>^ pixels, int width, int height) {
    pin_ptr<unsigned char> p = &pixels[0];
    engine->ApplyGrayscale(p, width, height);
}

void ImageProcessor::ApplyGaussianBlur(array<System::Byte>^ pixels, int width, int height, int sigma) {
    pin_ptr<unsigned char> p = &pixels[0];
    engine->ApplyGaussianBlur(p, width, height, sigma);
}

void ImageProcessor::ApplyMedianFilter(array<System::Byte>^ pixels, int width, int height, int kernelSize) {
    pin_ptr<unsigned char> p = &pixels[0];
    engine->ApplyMedianFilter(p, width, height, kernelSize);
}

void ImageProcessor::ApplyBinarization(array<System::Byte>^ pixels, int width, int height) {
    pin_ptr<unsigned char> p = &pixels[0];
    engine->ApplyBinarization(p, width, height);
}

void ImageProcessor::ApplyDilation(array<System::Byte>^ pixels, int width, int height) {
    pin_ptr<unsigned char> p = &pixels[0];
    engine->ApplyDilation(p, width, height);
}

void ImageProcessor::ApplyErosion(array<System::Byte>^ pixels, int width, int height) {
    pin_ptr<unsigned char> p = &pixels[0];
    engine->ApplyErosion(p, width, height);
}

void ImageProcessor::ApplySobel(array<System::Byte>^ pixels, int width, int height) {
    pin_ptr<unsigned char> p = &pixels[0];
    engine->ApplySobel(p, width, height);
}

void ImageProcessor::ApplyLaplacian(array<System::Byte>^ pixels, int width, int height) {
    pin_ptr<unsigned char> p = &pixels[0];
    engine->ApplyLaplacian(p, width, height);
}
