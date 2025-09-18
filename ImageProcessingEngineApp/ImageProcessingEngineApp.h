#pragma once

class ImageProcessingEngine {
	//���
	int stride;
	//�ʵ�
	unsigned int* editedImage; // ���ͷ�?
	//�Ӽ�
	ImageProcessingEngine();
	//������
	//�ۺ� �޼���
public:
	void ApplyGrayscale(unsigned char* data, int width, int height);
	void ApplyGaussianBlur(unsigned char* data, int width, int height, int sigma);
	void ApplyMedianFilter(unsigned char* data, int width, int height, int kernelSize);
	void ApplyBinarization(unsigned char* data, int width, int height);
	void ApplyDilation(unsigned char* data, int width, int height);
	void ApplyErosion(unsigned char* data, int width, int height);
	void ApplySobel(unsigned char* pixels, int width, int height);
	void ApplyLaplacian(unsigned char* pixels, int width, int height);
	void TemplateMatch(unsigned char* originalPixels, int width, int height, unsigned char* templatePixels, int templateWidth, int templateHeight, int* matchX, int* matchY);
	void ApplyFFT(unsigned char* data, int width, int height);
	void ApplyIFFT(unsigned char* data, int width, int height);

	//������Ʈ, �����̺� �޼���
	//��ø Ŭ����
};