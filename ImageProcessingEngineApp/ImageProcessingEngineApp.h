#pragma once

#include <iostream>
#include <cmath>
#include <numbers>
#include <vector>
#include <complex>
#include <algorithm>
#include <cstring>

#ifdef IMAGEPROCESSINGENGINEAPP_EXPORTS
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API __declspec(dllimport)
#endif

namespace NativeEngine {
	class ENGINE_API ImageProcessingEngine {
	private:

		//���
		//�ʵ�
		//�Ӽ�
		std::vector<std::vector<std::complex<double>>> _fftData;
		std::vector<std::vector<std::complex<double>>> _fftDataBackup;
		void fftShift();
		//������
		//�ۺ� �޼���
	public:
		void ApplyGrayscale(unsigned char* data, int width, int height);
		void ApplyGaussianBlur(unsigned char* data, int width, int height, float sigma);
		void ApplyMedian(unsigned char* data, int width, int height, int kernelSize);
		void ApplyBinarization(unsigned char* data, int width, int height);
		void ApplyDilation(unsigned char* data, int width, int height);
		void ApplyErosion(unsigned char* data, int width, int height);
		void ApplySobel(unsigned char* pixels, int width, int height);
		void ApplyLaplacian(unsigned char* pixels, int width, int height);
		void ApplyTemplateMatch(unsigned char* originalPixels, int width, int height, unsigned char* templatePixels, int templateWidth, int templateHeight, int* matchX, int* matchY);
		bool ApplyFFT(unsigned char* data, int width, int height);
		bool ApplyIFFT(unsigned char* data, int width, int height);
		void ClearFFTData();
		bool HasFFTData();

		//������Ʈ, �����̺� �޼���
		//��ø Ŭ����
	};
}
