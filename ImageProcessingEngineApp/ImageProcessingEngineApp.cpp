#include <iostream>
#include <cmath>
#include <numbers>
#include <vector>
#include <algorithm>
#include <cstring>
#include "ImageProcessingEngineApp.h"

void ImageProcessingEngine::ApplyGrayscale(unsigned char* pixels, int width, int height) {
	// 사진의 크기를 확인한다 -> 매개변수로 받음
	// 모든 픽셀의 RGB 값을 구한다
	unsigned int avg = 0;
	unsigned int red, blue, green;

	for (int i = 0; i < width * height; i++) {
		red = pixels[i * 3 + 0];
		green = pixels[i * 3 + 1];
		blue = pixels[i * 3 + 2];
		avg = (red + green + blue) / 3;
		pixels[i * 3 + 0] = avg;
		pixels[i * 3 + 1] = avg;
		pixels[i * 3 + 2] = avg;
	}
}

void ImageProcessingEngine::ApplyGaussianBlur(unsigned char* pixels, int width, int height, int sigma) {
	// 이미 그레이스케일이 적용되어있다고 가정
	// 가우시안 커널 계산
	std::vector<double> kernel(sigma * 8 + 1);
	double sum = 0.0;
	for (int i = -sigma * 4; i <= sigma * 4; i++) {
		double denom = sqrt(2 * std::numbers::pi * pow(sigma, 2)); // 분모
		double numerator = exp(-(i * i) / (2 * pow(sigma, 2)));
		kernel[i + sigma * 4] = numerator / denom;
		sum += kernel[i + sigma * 4];
	}
	//정규화
	for (double& val : kernel) val /= sum;

	// 임시 결과 저장
	std::vector<unsigned char> temp(width * height, 0);
	//가우시안 적용 - 가로 컨볼루션
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			double acc = 0.0;
			for (int k = -sigma * 4; k <= sigma * 4; k++) {
				// 경계처리 - clamp 사용
				int nx = std::clamp(x + k, 0, width - 1);
				acc += pixels[y * width + nx] * kernel[k + sigma * 4];
			}
			// 계산 결과 클램핑
			temp[y * width + x] = static_cast<unsigned char>(std::clamp(acc, 0.0, 255.0));
		}
	}
	memcpy(pixels, temp.data(), width * height);
}

void ImageProcessingEngine::ApplyMedianFilter(unsigned char* pixels, int width, int height, int kernelSize) {
	// 픽셀 순회하면서 커널 사이즈에 해당하는 영역 중에 중앙값 골라서 대체
	std::vector<unsigned char> result(width * height, 0);

	// 부드러운 이동을 위한 가로세로 이동
	for (int j = 0; j < height; j++) {  // 세로부터
		for (int i = 0; i < width; i++) {
			// 커널 단위로 처리
			std::vector<int> temp;
			for (int k = -(kernelSize / 2); k <= kernelSize / 2; k++) {
				for (int m = -(kernelSize / 2); m <= kernelSize / 2; m++) {
					int tempX = std::clamp(i + m, 0, width - 1);
					int tempY = std::clamp(j + k, 0, height - 1);
					temp.push_back(pixels[tempY * width + tempX]);
				}
			}
			//정렬해서 중앙값 뽑아내기
			std::sort(temp.begin(), temp.end());
			result[j * width + i] = temp[temp.size() / 2];
		}
	}
	for (int i = 0; i < width * height; i++) {
		pixels[i] = result[i];
	}
}

void ImageProcessingEngine::ApplyBinarization(unsigned char* pixels, int width, int height) {
	// 오츠 이진화 방식을 사용
	std::vector<int> histogram(256, 0);

	int sum = 0;
	// 히스토그램
	for (int i = 0; i < width * height; i++) {
		histogram[pixels[i]]++;
	}
	for (int i = 0; i < 256; i++) {
		sum += i * histogram[i];
	}
	//평균
	int avg = sum / (width * height);

	// 오츠 알고리즘 사용
	int threshold = 0;
	int maxVar = 0;
	for (int t = 0; t < 256; t++) { // 임계값의 기준이 되는 루프
		int sumA = 0, sumB = 0, weightA = 0, weightB = 0;
		for (int j = 0; j < 256; j++) {
			//그룹 나누기
			if (j < t) {//임계값이 더 크면
				sumA += j * histogram[j];
				weightA += histogram[j];
			}
			else {
				sumB += j * histogram[j];
				weightB += histogram[j];
			}
		}
		if (weightA == 0 || weightB == 0) continue;
		int meanA = sumA / weightA;
		int meanB = sumB / weightB;
		int varBetween = weightA * weightB * (meanA - meanB) * (meanA - meanB);
		if (varBetween > maxVar) {
			maxVar = varBetween;
			threshold = t;
		}
	}
	//임계값 가지고 이진화
	for (int i = 0; i < width * height; i++) {
		pixels[i] = (pixels[i] < threshold) ? 0 : 255;
	}
}

void ImageProcessingEngine::ApplyDilation(unsigned char* pixels, int width, int height) {
	std::vector<unsigned char> temp(width * height, 0);
	// 전체 노드 순회하기 (4 - 연결성)
	// 중심 픽셀 기준 커널 올려두기
	for (int i = 1; i < height - 1; i++) {
		for (int j = 1; j < width - 1; j++) {// 가로부터
			int idx = i * width + j;
			// 커널 단위로 처리, 가장자리는 빼고 안쪽만 처리	
			// 모든 픽셀이 다 하얀색이면 흰색으로 , 하나라도 검은색이면 검은색으로 두기
			if (pixels[idx] != 0) { //검은색이 아니면
				temp[idx] = 255;
			}
		}
	}
	memcpy(pixels, temp.data(), width * height);
}

void ImageProcessingEngine::ApplyErosion(unsigned char* pixels, int width, int height) {
	std::vector<unsigned char> temp(width * height, 255);
	// 전체 노드 순회하기 (4 - 연결성)
	// 중심 픽셀 기준 커널 올려두기
	for (int i = 1; i < height - 1; i++) {
		for (int j = 1; j < width - 1; j++) {// 가로부터
			int idx = i * width + j;
			// 커널 단위로 처리, 가장자리는 빼고 안쪽만 처리
			if (pixels[idx] == 0) { //흰색이 아니면
				temp[idx] = 0;
			}
		}
	}
	// 모든 픽셀이 다 하얀색이면 흰색으로 , 하나라도 검은색이면 검은색으로 두기
	memcpy(pixels, temp.data(), width * height);
}
void ImageProcessingEngine::ApplySobel(unsigned char* pixels, int width, int height) {
	// 결과를 저장할 버퍼
	std::vector<unsigned char> result(width * height, 0);
	// x, y 방향 그래디언트를 임시 저장할 버퍼
	std::vector<double> gradientX(width * height, 0);
	std::vector<double> gradientY(width * height, 0);

	// 수평 커널 (Gx)
	int kernelX[3][3] = {
		{-1, 0, 1},
		{-2, 0, 2},
		{-1, 0, 1}
	};

	// 수직 커널 (Gy)
	int kernelY[3][3] = {
		{ 1,  2,  1},
		{ 0,  0,  0},
		{-1, -2, -1}
	};

	// 경계(1픽셀)를 제외하고 컨볼루션 연산 수행
	for (int i = 1; i < height - 1; i++) {
		for (int j = 1; j < width - 1; j++) {
			double sumX = 0;
			double sumY = 0;

			for (int k = -1; k <= 1; k++) {
				for (int m = -1; m <= 1; m++) {
					int pixelVal = pixels[(i + k) * width + (j + m)];
					sumX += pixelVal * kernelX[k + 1][m + 1];
					sumY += pixelVal * kernelY[k + 1][m + 1];
				}
			}
			gradientX[i * width + j] = sumX;
			gradientY[i * width + j] = sumY;
		}
	}

	// 결과 magnitude 계산
	for (int i = 1; i < height - 1; i++) {
		for (int j = 1; j < width - 1; j++) {
			double magnitude = sqrt(pow(gradientX[i * width + j], 2) + pow(gradientY[i * width + j], 2));
			result[i * width + j] = static_cast<unsigned char>(std::clamp(magnitude, 0.0, 255.0));
		}
	}

	// 경계 픽셀은 원본 유지
	for (int i = 0; i < width; i++) {
		result[i] = pixels[i];                          // top row
		result[(height - 1) * width + i] = pixels[(height - 1) * width + i]; // bottom row
	}
	for (int i = 0; i < height; i++) {
		result[i * width] = pixels[i * width];          // left col
		result[i * width + (width - 1)] = pixels[i * width + (width - 1)]; // right col
	}

	// 결과를 원본으로 복사
	memcpy(pixels, result.data(), width * height);
}

void ImageProcessingEngine::ApplyLaplacian(unsigned char* pixels, int width, int height) {
	std::vector<unsigned char> result(width * height, 0);
	//라플라시안 커널
	int kernel[3][3] = {
		{0, 1, 0},
		{1, -4, 1},
		{0, 1, 0}
	};
	//패딩처리

	//중심 픽셀에 대한 계산
	for (int j = 1; j < height - 1; j++) {
		for (int i = 1; i < width - 1; i++) {
			//주변 픽셀 정의
			int current = pixels[j * width + i];
			int up = pixels[(j - 1) * width + i];
			int down = pixels[(j + 1) * width + i];
			int left = pixels[j * width + (i - 1)];
			int right = pixels[j * width + (i + 1)];

			int temp = up + down + left + right - 4 * current;
			result[j * width + i] = static_cast<unsigned char>(
				std::clamp(temp, 0, 255)
				);
		}
	}
	memcpy(pixels, result.data(), width * height);
}

void ImageProcessingEngine::TemplateMatch(
	unsigned char* originalPixels, int originalWidth, int originalHeight,
	unsigned char* templatePixels, int templateWidth, int templateHeight,
	int* matchX, int* matchY)
{
	//전체 순회하기
	std::vector<unsigned int> result(originalWidth * originalHeight);
	int temp = 0;
	int min = 256;
	for (int j = 0; j < originalHeight - templateHeight; j++) {
		for (int i = 0; i < originalWidth - templateWidth; i++) { // 가로 -> 세로 순서

			for (int tj = 0; tj < templateHeight; tj++) {
				for (int ti = 0; ti < templateWidth; ti++) {
					// 픽셀 접근
					int origIdx = (j + tj) * originalWidth + (i + ti);
					int tempIdx = tj * templateWidth + ti;

					temp = abs(originalPixels[origIdx] - templatePixels[tempIdx]);
					result[origIdx] = temp;

					if (temp < min) {
						min = temp;
						*matchX = i;
						*matchY = j;
					}
				}
			}
		}
	}
}

void ApplyFFT(unsigned char* data, int width, int height) {
	// 
}
void ApplyIFFT(unsigned char* data, int width, int height) {

}