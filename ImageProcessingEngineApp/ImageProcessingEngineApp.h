#pragma once

class ImageProcessingEngine {
	//상수
	int stride;
	//필드
	unsigned int* editedImage; // 벡터로?
	//속성
	ImageProcessingEngine();
	//생성자
	//퍼블릭 메서드
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

	//프로텍트, 프라이빗 메서드
	//중첩 클래스
};