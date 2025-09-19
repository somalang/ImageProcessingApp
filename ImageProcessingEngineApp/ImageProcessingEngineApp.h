#pragma once

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
		void ApplyFFT(unsigned char* data, int width, int height);
		void ApplyIFFT(unsigned char* data, int width, int height);
		void ClearFFTData();
		bool HasFFTData();

		//������Ʈ, �����̺� �޼���
		//��ø Ŭ����
	};
}
