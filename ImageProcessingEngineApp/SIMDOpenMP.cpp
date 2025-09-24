#include <omp.h>
#include <windows.h>
#include <string>
#include "ImageProcessingEngineApp.h"

using namespace std;

void NativeEngine::ImageProcessingEngine::ApplyGrayscale(unsigned char* pixels, int width, int height) {
	// 사진의 크기를 확인한다 -> 매개변수로 받음
	// 모든 픽셀의 RGB 값을 구한다
	unsigned int avg = 0;
	unsigned int red, blue, green;

#pragma omp parallel for private(red, green, blue, avg)
	for (int i = 0; i < width * height; i++) {
		red = pixels[i * 4 + 0];
		green = pixels[i * 4 + 1];
		blue = pixels[i * 4 + 2];
		avg = (red + green + blue) / 3;
		pixels[i * 4 + 0] = avg;
		pixels[i * 4 + 1] = avg;
		pixels[i * 4 + 2] = avg;

		if (i < 20) {
			std::string msg = "Pixel " + std::to_string(i) +
				" processed by thread " + std::to_string(omp_get_thread_num()) + "\n";
			OutputDebugStringA(msg.c_str());
		}
	}
}

//명도는 유지한다 (alpha)
void NativeEngine::ImageProcessingEngine::ApplyGaussianBlur(
	unsigned char* pixels, int width, int height, float sigma)
{
	const int channels = 4; // BGRA 포맷
	const int stride = width * channels;
	const int dataSize = stride * height;

	// WPF 배열을 안전하게 임시 벡터로 복사
	std::vector<unsigned char> inputBuffer(pixels, pixels + dataSize);
	std::vector<unsigned char> tempBuffer(dataSize);

	// 가우시안 커널 계산
	int kernelRadius = static_cast<int>(ceil(sigma * 3));
	int kernelSize = kernelRadius * 2 + 1;
	std::vector<double> kernel(kernelSize);
	double sum = 0.0;
	for (int i = 0; i < kernelSize; i++) {
		kernel[i] = exp(-0.5 * pow((i - kernelRadius) / sigma, 2.0));
		sum += kernel[i];
	}
	for (int i = 0; i < kernelSize; i++) kernel[i] /= sum;

	// ---------------------
	// 가로 블러 적용 (병렬)
	// ---------------------
#pragma omp parallel for schedule(static)
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			double b = 0.0, g = 0.0, r = 0.0;
			for (int k = -kernelRadius; k <= kernelRadius; ++k) {
				int nx = std::clamp(x + k, 0, width - 1);
				int idx = y * stride + nx * channels;
				double weight = kernel[k + kernelRadius];
				b += inputBuffer[idx + 0] * weight;
				g += inputBuffer[idx + 1] * weight;
				r += inputBuffer[idx + 2] * weight;
			}
			int out_idx = y * stride + x * channels;
			tempBuffer[out_idx + 0] = static_cast<unsigned char>(std::clamp(b, 0.0, 255.0));
			tempBuffer[out_idx + 1] = static_cast<unsigned char>(std::clamp(g, 0.0, 255.0));
			tempBuffer[out_idx + 2] = static_cast<unsigned char>(std::clamp(r, 0.0, 255.0));
			tempBuffer[out_idx + 3] = inputBuffer[out_idx + 3]; // 알파 유지
		}
	}

	// ---------------------
	// 세로 블러 적용 (병렬)
	// ---------------------
#pragma omp parallel for schedule(static)
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			double b = 0.0, g = 0.0, r = 0.0;
			for (int k = -kernelRadius; k <= kernelRadius; ++k) {
				int ny = std::clamp(y + k, 0, height - 1);
				int idx = ny * stride + x * channels;
				double weight = kernel[k + kernelRadius];
				b += tempBuffer[idx + 0] * weight;
				g += tempBuffer[idx + 1] * weight;
				r += tempBuffer[idx + 2] * weight;
			}
			int out_idx = y * stride + x * channels;
			pixels[out_idx + 0] = static_cast<unsigned char>(std::clamp(b, 0.0, 255.0));
			pixels[out_idx + 1] = static_cast<unsigned char>(std::clamp(g, 0.0, 255.0));
			pixels[out_idx + 2] = static_cast<unsigned char>(std::clamp(r, 0.0, 255.0));
			// 알파 채널 그대로
		}
	}
}


void NativeEngine::ImageProcessingEngine::ApplyMedian(unsigned char* pixels, int width, int height, int kernelSize) {
	const int channels = 4;
	const int stride = width * channels;
	const int dataSize = stride * height;

	std::vector<unsigned char> result(dataSize);
	int kernelHalf = kernelSize / 2;
	int kernelArea = kernelSize * kernelSize;

	// median_index를 미리 계산 (한 번만)
	const size_t medianIndex = static_cast<size_t>(kernelArea / 2);

	// OpenMP 병렬 처리 영역을 루프 바깥으로 확장
#pragma omp parallel
	{
		// 각 스레드가 사용할 벡터를 한 번만 선언 (메모리 재사용)
		std::vector<unsigned char> blue;
		std::vector<unsigned char> green;
		std::vector<unsigned char> red;

		// 미리 공간을 할당하여 루프 내에서의 재할당 방지
		blue.reserve(kernelArea);
		green.reserve(kernelArea);
		red.reserve(kernelArea);

		// 스레드들이 y 루프를 나누어 처리
#pragma omp for schedule(dynamic,4)
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				// 벡터를 재사용하기 전에 이전 데이터를 지움
				blue.clear();
				green.clear();
				red.clear();

				// 커널 순회
				for (int ky = -kernelHalf; ky <= kernelHalf; ky++) {
					for (int kx = -kernelHalf; kx <= kernelHalf; kx++) {
						int nx = std::clamp(x + kx, 0, width - 1);
						int ny = std::clamp(y + ky, 0, height - 1);
						int neighbor_idx = ny * stride + nx * channels;

						blue.push_back(pixels[neighbor_idx + 0]);
						green.push_back(pixels[neighbor_idx + 1]);
						red.push_back(pixels[neighbor_idx + 2]);
					}
				}

				// 중앙값 찾기: 전체 정렬 대신 nth_element 사용
				std::nth_element(blue.begin(),
					blue.begin() + medianIndex,
					blue.end());
				std::nth_element(green.begin(),
					green.begin() + medianIndex,
					green.end());
				std::nth_element(red.begin(),
					red.begin() + medianIndex,
					red.end());

				int current_idx = y * stride + x * channels;

				result[current_idx + 0] = blue[medianIndex];
				result[current_idx + 1] = green[medianIndex];
				result[current_idx + 2] = red[medianIndex];
				result[current_idx + 3] = pixels[current_idx + 3];
			}
		}
	} // 병렬 처리 영역 종료

	// 최종 결과를 원본 픽셀 버퍼로 복사
	memcpy(pixels, result.data(), dataSize);
}

void NativeEngine::ImageProcessingEngine::ApplyBinarization(unsigned char* pixels, int width, int height) {
	const int channels = 4;
	const int stride = width * channels;
	const int pixelCount = width * height;

	std::vector<unsigned char> gray(pixelCount);
	std::vector<int> histogram(256, 0);

	// 1. RGB → Grayscale 변환 (병렬 처리)
#pragma omp parallel for
	for (int i = 0; i < pixelCount; ++i) {
		gray[i] = static_cast<unsigned char>(
			0.114 * pixels[i * channels + 0] +  // Blue
			0.587 * pixels[i * channels + 1] +  // Green
			0.299 * pixels[i * channels + 2]    // Red
			);
	}

	// 히스토그램 계산
#pragma omp parallel
	{
		// 스레드별 로컬 히스토그램을 생성
		std::vector<int> local_histogram(256, 0);

		// 스레드 -> 로컬 히스토그램에 저장
#pragma omp for nowait
		for (int i = 0; i < pixelCount; i++) {
			local_histogram[gray[i]]++;
		}

		// 로컬 -> 전체 히스토그램
#pragma omp critical
		for (int i = 0; i < 256; i++) {
			histogram[i] += local_histogram[i];
		}
	}

	// 최적 임계값 계산
	float totalSum = 0.0f;
	for (int i = 0; i < 256; i++) {
		totalSum += i * histogram[i];
	}

	float sumForeground = 0.0f;
	int weightForeground = 0;
	int weightBackground = 0;

	float maxVariance = 0.0f;
	int optimalThreshold = 0;

	for (int t = 0; t < 256; t++) {
		weightForeground += histogram[t];
		if (weightForeground == 0) continue;

		weightBackground = pixelCount - weightForeground;
		if (weightBackground == 0) break;

		sumForeground += static_cast<float>(t * histogram[t]);

		float meanForeground = sumForeground / weightForeground;
		float meanBackground = (totalSum - sumForeground) / weightBackground;

		float varianceBetween = static_cast<float>(weightForeground) *
			static_cast<float>(weightBackground) *
			(meanForeground - meanBackground) *
			(meanForeground - meanBackground);

		if (varianceBetween > maxVariance) {
			maxVariance = varianceBetween;
			optimalThreshold = t;
		}
	}

	// 최적 임계값
#pragma omp parallel for
	for (int i = 0; i < pixelCount; ++i) {
		unsigned char value = (gray[i] > optimalThreshold) ? 255 : 0;
		pixels[i * channels + 0] = value;
		pixels[i * channels + 1] = value;
		pixels[i * channels + 2] = value;
	}
}

void NativeEngine::ImageProcessingEngine::ApplyDilation(unsigned char* pixels, int width, int height) {
	const int channels = 4;
	const int stride = width * channels;
	const int dataSize = stride * height;
	std::vector<unsigned char> temp(dataSize);
	memcpy(temp.data(), pixels, dataSize); // 원본 이미지를 temp에 복사

	// 가장자리를 제외한 픽셀 순회
#pragma omp parallel for
	for (int y = 1; y < height - 1; y++) {
		for (int x = 1; x < width - 1; x++) {
			int current = y * stride + x * channels;
			unsigned char max = 0;

			// 3x3 커널 순회
			for (int ky = -1; ky <= 1; ky++) {
				for (int kx = -1; kx <= 1; kx++) {
					int neighborIdx = (y + ky) * stride + (x + kx) * channels;
					// B 채널 기준 가장 밝은 값
					if (temp[neighborIdx] > max) {
						max = temp[neighborIdx];
					}
				}
			}

			//최대값 설정
			pixels[current + 0] = max;
			pixels[current + 1] = max;
			pixels[current + 2] = max;
		}
	}
}

void NativeEngine::ImageProcessingEngine::ApplyErosion(unsigned char* pixels, int width, int height) {
	const int channels = 4;
	const int stride = width * channels;
	const int dataSize = stride * height;
	std::vector<unsigned char> temp(dataSize);
	memcpy(temp.data(), pixels, dataSize);

	// 가장자리 제외 픽셀 순회
#pragma omp parallel for
	for (int y = 1; y < height - 1; y++) {
		for (int x = 1; x < width - 1; x++) {
			int current_idx = y * stride + x * channels;
			unsigned char min = 255;

			// 3x3 커널 순회
			for (int ky = -1; ky <= 1; ky++) {
				for (int kx = -1; kx <= 1; kx++) {
					int neighbor_idx = (y + ky) * stride + (x + kx) * channels;
					// B 기준 제일 어두운값
					if (temp[neighbor_idx] < min) {
						min = temp[neighbor_idx];
					}
				}
			}

			// 최소값 설정
			pixels[current_idx + 0] = min;
			pixels[current_idx + 1] = min;
			pixels[current_idx + 2] = min;
		}
	}
}

void NativeEngine::ImageProcessingEngine::ApplySobel(unsigned char* pixels, int width, int height) {
	const int channels = 4;
	const int stride = width * channels;
	const int pixelNum = width * height;

	// 그레이스케일 적용해서 임시버퍼 넣기
	std::vector<unsigned char> gray_pixels(pixelNum);
#pragma omp parallel for
	for (int i = 0; i < pixelNum; ++i) {
		gray_pixels[i] = static_cast<unsigned char>(
			0.114 * pixels[i * channels + 0] + // Blue
			0.587 * pixels[i * channels + 1] + // Green
			0.299 * pixels[i * channels + 2]   // Red
			);
	}

	// 그래디언트와 최종 결과를 저장할 버퍼
	std::vector<unsigned char> result(pixelNum, 0);
	std::vector<double> gradientX(pixelNum, 0);
	std::vector<double> gradientY(pixelNum, 0);

	// 소벨 커널
	int kernelX[3][3] = { {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1} };
	int kernelY[3][3] = { {1, 2, 1}, {0, 0, 0}, {-1, -2, -1} };

	// 컨볼루션 연산
	for (int y = 1; y < height - 1; y++) {
		for (int x = 1; x < width - 1; x++) {
			double sumX = 0;
			double sumY = 0;
			for (int ky = -1; ky <= 1; ky++) {
				for (int kx = -1; kx <= 1; kx++) {
					int pixelVal = gray_pixels[(y + ky) * width + (x + kx)];
					sumX += pixelVal * kernelX[ky + 1][kx + 1];
					sumY += pixelVal * kernelY[ky + 1][kx + 1];
				}
			}
			gradientX[y * width + x] = sumX;
			gradientY[y * width + x] = sumY;
		}
	}

	// 최종 그래디언트 적용 이미지
#pragma omp parallel for
	for (int i = 0; i < pixelNum; i++) {
		double magnitude = sqrt(pow(gradientX[i], 2) + pow(gradientY[i], 2));
		result[i] = static_cast<unsigned char>(std::clamp(magnitude, 0.0, 255.0));
	}

	// 원본에 복사
#pragma omp parallel for
	for (int i = 0; i < pixelNum; ++i) {
		pixels[i * channels + 0] = result[i];
		pixels[i * channels + 1] = result[i];
		pixels[i * channels + 2] = result[i];
	}
}

void NativeEngine::ImageProcessingEngine::ApplyLaplacian(unsigned char* pixels, int width, int height) {
	const int channels = 4;
	const int stride = width * channels;
	const int pixelNum = width * height;

	// 그레이스케일 변환
	std::vector<unsigned char> temp(pixelNum);

#pragma omp parallel for
	for (int i = 0; i < pixelNum; ++i) {
		temp[i] = static_cast<unsigned char>(
			0.114 * pixels[i * channels + 0] + // Blue
			0.587 * pixels[i * channels + 1] + // Green
			0.299 * pixels[i * channels + 2]   // Red
			);
	}

	// 라플라스 연산 결과 저장
	std::vector<int> laplacianResult(pixelNum, 0);

	// 8방향 사용
#pragma omp parallel for
	for (int y = 1; y < height - 1; y++) {
		for (int x = 1; x < width - 1; x++) {
			int current_idx = y * width + x;

			int upLeft = temp[(y - 1) * width + (x - 1)];
			int up = temp[(y - 1) * width + x];
			int upRight = temp[(y - 1) * width + (x + 1)];
			int left = temp[y * width + (x - 1)];
			int current = temp[current_idx];
			int right = temp[y * width + (x + 1)];
			int downLeft = temp[(y + 1) * width + (x - 1)];
			int down = temp[(y + 1) * width + x];
			int downRight = temp[(y + 1) * width + (x + 1)];

			// 8개 픽셀 합 - 8 * 중앙값
			int laplacianValue = (upLeft + up + upRight + left + right + downLeft + down + downRight) - 8 * current;

			// 절댓값 -> 엣지 시각화
			laplacianResult[current_idx] = abs(laplacianValue);
		}
	}

	// 정규화
	int max = 0;
	for (int val : laplacianResult) {
		if (val > max) {
			max = val;
		}
	}
	// 0 나누기 방지
	if (max == 0) {
		max = 1;
	}

	// 정규화 결과저장
#pragma omp parallel for
	for (int i = 0; i < pixelNum; ++i) {
		unsigned char result = static_cast<unsigned char>((laplacianResult[i] * 255) / max);
		pixels[i * channels + 0] = result; // Blue
		pixels[i * channels + 1] = result; // Green
		pixels[i * channels + 2] = result; // Red
		// Alpha 채널은 그대로 유지
	}
}

void NativeEngine::ImageProcessingEngine::ApplyTemplateMatch(
	unsigned char* originalPixels, int originalWidth, int originalHeight,
	unsigned char* templatePixels, int templateWidth, int templateHeight,
	int* matchX, int* matchY)
{
	const int channels = 4;

	// 원본 그레이스케일 변환
	int originalPixelNum = originalWidth * originalHeight;
	std::vector<unsigned char> originalGrayScaled(originalPixelNum);

#pragma omp parallel for
	for (int i = 0; i < originalPixelNum; ++i) {
		originalGrayScaled[i] = static_cast<unsigned char>(
			(originalPixels[i * channels] + originalPixels[i * channels + 1] + originalPixels[i * channels + 2]) / 3.0
			);
	}

	// 템플릿도 변환
	int templatePixelNum = templateWidth * templateHeight;
	std::vector<unsigned char> templateGrayScaled(templatePixelNum);

#pragma omp parallel for
	for (int i = 0; i < templatePixelNum; ++i) {
		templateGrayScaled[i] = static_cast<unsigned char>(
			(templatePixels[i * channels] + templatePixels[i * channels + 1] + templatePixels[i * channels + 2]) / 3.0
			);
	}

	long long min = -1; // 최종 최솟값을 저장할 변수
	int finalMatchX = -1;
	int finalMatchY = -1;

	// 템플릿 매칭 수행 (병렬 처리)
#pragma omp parallel
	{
		long long localMin = -1; // 각 스레드가 독립적으로 사용할 지역 변수
		int localMatchX = -1;
		int localMatchY = -1;

		// y 루프를 스레드들이 나누어 처리하도록 설정
#pragma omp for
		for (int y = 0; y <= originalHeight - templateHeight; y++) {
			for (int x = 0; x <= originalWidth - templateWidth; x++) {
				long long currentSAD = 0;

				// 템플릿 영역 순회 -> 절대차 합계(SAD) 계산
				for (int ty = 0; ty < templateHeight; ty++) {
					for (int tx = 0; tx < templateWidth; tx++) {
						int originalIdx = (y + ty) * originalWidth + (x + tx);
						int templateIdx = ty * templateWidth + tx;

						currentSAD += abs(originalGrayScaled[originalIdx] - templateGrayScaled[templateIdx]);
					}
				}

				// 각 스레드는 자신의 담당 구역에서만 최소값을 찾음
				if (localMin == -1 || currentSAD < localMin) {
					localMin = currentSAD;
					localMatchX = x;
					localMatchY = y;
				}
			}
		}

		// 모든 스레드가 각자의 계산을 마친 후, 결과를 취합
#pragma omp critical
		{
			if (min == -1 || localMin < min) {
				min = localMin;
				finalMatchX = localMatchX;
				finalMatchY = localMatchY;
			}
		}
	} // 병렬 처리 영역 종료

	// 최종 결과를 포인터를 통해 반환
	*matchX = finalMatchX;
	*matchY = finalMatchY;
}

void fft1d(vector<complex<double>>& data, bool inverse = false) {
	int num = data.size();
	if (num <= 1) return;

	// 비트 반전 순서로 데이터 재정렬
	for (int i = 1, j = 0; i < num; i++) {
		int bit = num >> 1;
		for (; j & bit; bit >>= 1)
			j ^= bit;
		j ^= bit;
		if (i < j)
			swap(data[i], data[j]);
	}

	// 버터플라이 연산
	for (int len = 2; len <= num; len <<= 1) {
		double ang = 2 * std::numbers::pi / len * (inverse ? -1 : 1);
		complex<double> wlen(cos(ang), sin(ang));
#pragma omp parallel for
		for (int i = 0; i < num; i += len) {
			complex<double> w(1);
			for (int j = 0; j < len / 2; j++) {
				complex<double> u = data[i + j];
				complex<double> v = data[i + j + len / 2] * w;
				data[i + j] = u + v;
				data[i + j + len / 2] = u - v;
				w *= wlen;
			}
		}
	}

	// IFFT일 경우 크기 보정
	if (inverse) {
#pragma omp parallel for
		for (int i = 0; i < num; i++) {
			data[i] /= num;
		}
	}
}

//shift 가 빠짐!
void NativeEngine::ImageProcessingEngine::fftShift() {
	int height = _fftData.size();
	int width = _fftData[0].size();
	int cx = width / 2;
	int cy = height / 2;

#pragma omp parallel for
	for (int y = 0; y < cy; y++) {
		for (int x = 0; x < cx; x++) {
			swap(_fftData[y][x], _fftData[y + cy][x + cx]);
			swap(_fftData[y + cy][x], _fftData[y][x + cx]);
		}
	}
}

//패딩연산은 그냥 함수로 빼기
int nextPowerOf2(int n) {
	int p = 1;
	while (p < n) p <<= 1;
	return p;
}

bool NativeEngine::ImageProcessingEngine::ApplyFFT(unsigned char* pixels, int width, int height) {
	const int channels = 4; // BGRA

	// 패딩하기
	int padWidth = nextPowerOf2(width);
	int padHeight = nextPowerOf2(height);

	_fftData.assign(padHeight, vector<complex<double>>(padWidth));

	//그레이스케일 변환, 제로 패딩 적용
#pragma omp parallel for
	for (int j = 0; j < padHeight; j++) {
		for (int i = 0; i < padWidth; i++) {
			if (j < height && i < width) {
				int idx = (j * width + i) * channels;
				double gray = 0.114 * pixels[idx + 0] + 0.587 * pixels[idx + 1] + 0.299 * pixels[idx + 2];
				_fftData[j][i] = complex<double>(gray, 0.0);
			}
			else {
				_fftData[j][i] = complex<double>(0.0, 0.0); // Zero-padding
			}
		}
	}

	//2D FFT 
#pragma omp parallel for
	for (int j = 0; j < padHeight; j++) fft1d(_fftData[j], false);
	for (int i = 0; i < padWidth; i++) {
		vector<complex<double>> col(padHeight);
		for (int j = 0; j < padHeight; j++) col[j] = _fftData[j][i];
		fft1d(col, false);
		for (int j = 0; j < padHeight; j++) _fftData[j][i] = col[j];
	}

	_fftDataBackup = _fftData;

	// 중앙으로 이동
	fftShift();

	//로그 스케일링 -> 명암 대비 조절해야 함
	double max_val = 0.0;
	vector<vector<double>> mag(padHeight, vector<double>(padWidth));
#pragma omp parallel for
	for (int j = 0; j < padHeight; j++) {
		for (int i = 0; i < padWidth; i++) {
			double val = log(1.0 + abs(_fftData[j][i]));
			mag[j][i] = val;
			if (val > max_val) max_val = val;
		}
	}

	//정규화(0~255)해서 저장
	if (max_val == 0) max_val = 1.0;
	int startX = (padWidth - width) / 2;
	int startY = (padHeight - height) / 2;
#pragma omp parallel for
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			unsigned char v = static_cast<unsigned char>((mag[j + startY][i + startX] / max_val) * 255.0);
			int idx = (j * width + i) * channels;
			pixels[idx + 0] = v; // Blue
			pixels[idx + 1] = v; // Green
			pixels[idx + 2] = v; // Red
			pixels[idx + 3] = 255; // Alpha (불투명)
		}
	}

	return !_fftData.empty() && !_fftData[0].empty();
}

bool NativeEngine::ImageProcessingEngine::ApplyIFFT(unsigned char* pixels, int width, int height) {
	if (_fftDataBackup.empty()) return false;

	const int channels = 4;
	const int padHeight = _fftDataBackup.size();
	const int padWidth = _fftDataBackup[0].size();

	// 저장해둔 데이터 사용 (shift 전)
	_fftData = _fftDataBackup;

	// 2D IFFT (가로 -> 세로)
#pragma omp parallel for
	for (int y = 0; y < padHeight; y++) fft1d(_fftData[y], true);
	for (int x = 0; x < padWidth; x++) {
		vector<complex<double>> col(padHeight);
		for (int y = 0; y < padHeight; y++) col[y] = _fftData[y][x];
		fft1d(col, true);
		for (int y = 0; y < padHeight; y++) _fftData[y][x] = col[y];
	}

	// 원본 이미지 크기에 맞게 복원 이미지 잘라서 저장
#pragma omp parallel for
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			double val = _fftData[y][x].real();
			unsigned char gray = static_cast<unsigned char>(std::clamp(round(val), 0.0, 255.0));

			//BGRA 처리하기
			int idx = (y * width + x) * channels;
			pixels[idx + 0] = gray;
			pixels[idx + 1] = gray;
			pixels[idx + 2] = gray;
			pixels[idx + 3] = 255;
		}
	}

	//사용 끝난 후 초기화하기
	ClearFFTData();

	return true;
}

void NativeEngine::ImageProcessingEngine::ClearFFTData() {
	_fftDataBackup.clear();
	_fftData.clear();
}

bool NativeEngine::ImageProcessingEngine::HasFFTData() {
	return !_fftDataBackup.empty() && !_fftDataBackup[0].empty();
}

