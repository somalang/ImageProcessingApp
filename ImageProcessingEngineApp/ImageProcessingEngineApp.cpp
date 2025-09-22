#include <fstream>

#include "ImageProcessingEngineApp.h"

using namespace std;

void NativeEngine::ImageProcessingEngine::ApplyGrayscale(unsigned char* pixels, int width, int height) {
	// 사진의 크기를 확인한다 -> 매개변수로 받음
	// 모든 픽셀의 RGB 값을 구한다
	unsigned int avg = 0;
	unsigned int red, blue, green;

	for (int i = 0; i < width * height; i++) {
		red = pixels[i * 4 + 0];
		green = pixels[i * 4 + 1];
		blue = pixels[i * 4 + 2];
		avg = (red + green + blue) / 3;
		pixels[i * 4 + 0] = avg;
		pixels[i * 4 + 1] = avg;
		pixels[i * 4 + 2] = avg;
	}
}

void NativeEngine::ImageProcessingEngine::ApplyGaussianBlur(unsigned char* pixels, int width, int height, float sigma) {
	const int channels = 4; // BGRA 포맷이므로 4채널
	const int stride = width * channels;
	const int dataSize = stride * height;

	// 가우시안 커널 계산
	int kernelRadius = static_cast<int>(ceil(sigma * 3));
	int kernelSize = kernelRadius * 2 + 1;
	std::vector<double> kernel(kernelSize);
	double sum = 0.0;

	for (int i = 0; i < kernelSize; i++) {
		kernel[i] = exp(-0.5 * pow((i - kernelRadius) / sigma, 2.0));
		sum += kernel[i];
	}
	// 정규화
	for (int i = 0; i < kernelSize; i++) {
		kernel[i] /= sum;
	}

	std::vector<unsigned char> temp(dataSize);

	// 1. 수평 방향 블러 적용
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			double b = 0.0, g = 0.0, r = 0.0;
			for (int k = -kernelRadius; k <= kernelRadius; ++k) {
				int nx = std::clamp(x + k, 0, width - 1);
				int idx = y * stride + nx * channels;
				double weight = kernel[k + kernelRadius];
				b += pixels[idx + 0] * weight;
				g += pixels[idx + 1] * weight;
				r += pixels[idx + 2] * weight;
			}
			int out_idx = y * stride + x * channels;
			temp[out_idx + 0] = static_cast<unsigned char>(std::clamp(b, 0.0, 255.0));
			temp[out_idx + 1] = static_cast<unsigned char>(std::clamp(g, 0.0, 255.0));
			temp[out_idx + 2] = static_cast<unsigned char>(std::clamp(r, 0.0, 255.0));
			temp[out_idx + 3] = pixels[out_idx + 3]; // 알파 채널은 그대로 유지
		}
	}

	// 2. 수직 방향 블러 적용
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			double b = 0.0, g = 0.0, r = 0.0;
			for (int k = -kernelRadius; k <= kernelRadius; ++k) {
				int ny = std::clamp(y + k, 0, height - 1);
				int idx = ny * stride + x * channels;
				double weight = kernel[k + kernelRadius];
				b += temp[idx + 0] * weight;
				g += temp[idx + 1] * weight;
				r += temp[idx + 2] * weight;
			}
			int out_idx = y * stride + x * channels;
			pixels[out_idx + 0] = static_cast<unsigned char>(std::clamp(b, 0.0, 255.0));
			pixels[out_idx + 1] = static_cast<unsigned char>(std::clamp(g, 0.0, 255.0));
			pixels[out_idx + 2] = static_cast<unsigned char>(std::clamp(r, 0.0, 255.0));
			// 알파 채널은 이미 temp에 복사되어 있으므로 따로 처리할 필요 없음
		}
	}
}

void NativeEngine::ImageProcessingEngine::ApplyMedian(unsigned char* pixels, int width, int height, int kernelSize) {
	const int channels = 4; // BGRA 포맷
	const int stride = width * channels;
	const int dataSize = stride * height;

	// 최종 결과를 저장할 버퍼. 크기를 4채널에 맞게 수정합니다.
	std::vector<unsigned char> result(dataSize);

	int kernelHalf = kernelSize / 2;

	// 이미지의 각 픽셀을 순회합니다.
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			// 각 채널(B, G, R)의 이웃 픽셀 값들을 저장할 벡터
			std::vector<unsigned char> neighborhood_b;
			std::vector<unsigned char> neighborhood_g;
			std::vector<unsigned char> neighborhood_r;

			// 커널(마스크)을 순회하며 이웃 픽셀 값을 수집합니다.
			for (int ky = -kernelHalf; ky <= kernelHalf; ky++) {
				for (int kx = -kernelHalf; kx <= kernelHalf; kx++) {
					// 경계 처리
					int nx = std::clamp(x + kx, 0, width - 1);
					int ny = std::clamp(y + ky, 0, height - 1);

					// 4채널을 고려한 인덱스 계산
					int neighbor_idx = ny * stride + nx * channels;

					// 각 채널의 값을 해당 벡터에 추가
					neighborhood_b.push_back(pixels[neighbor_idx + 0]);
					neighborhood_g.push_back(pixels[neighbor_idx + 1]);
					neighborhood_r.push_back(pixels[neighbor_idx + 2]);
				}
			}

			// 각 채널별로 정렬하여 중앙값을 찾습니다.
			std::sort(neighborhood_b.begin(), neighborhood_b.end());
			std::sort(neighborhood_g.begin(), neighborhood_g.end());
			std::sort(neighborhood_r.begin(), neighborhood_r.end());

			// 4채널을 고려한 현재 픽셀의 인덱스
			int current_idx = y * stride + x * channels;

			// 중앙값으로 결과 버퍼를 채웁니다.
			size_t median_index = neighborhood_b.size() / 2;
			result[current_idx + 0] = neighborhood_b[median_index]; // Blue
			result[current_idx + 1] = neighborhood_g[median_index]; // Green
			result[current_idx + 2] = neighborhood_r[median_index]; // Red
			result[current_idx + 3] = pixels[current_idx + 3]; // Alpha 채널은 원본 값 유지
		}
	}

	// 처리된 결과(result)를 원본 픽셀 버퍼(pixels)로 복사합니다.
	memcpy(pixels, result.data(), dataSize);
}

void NativeEngine::ImageProcessingEngine::ApplyBinarization(unsigned char* pixels, int width, int height) {
	const int channels = 4;
	const int stride = width * channels;
	const int pixel_count = width * height;

	// 1. BGRA 이미지를 그레이스케일로 변환하여 임시 버퍼에 저장
	std::vector<unsigned char> gray_pixels(pixel_count);
	for (int i = 0; i < pixel_count; ++i) {
		// 밝기 평균 (Luminosity-preserving)
		gray_pixels[i] = static_cast<unsigned char>(
			0.114 * pixels[i * channels + 0] + // Blue
			0.587 * pixels[i * channels + 1] + // Green
			0.299 * pixels[i * channels + 2]   // Red
			);
	}

	// 2. 그레이스케일 이미지로 히스토그램 생성 및 Otsu 임계값 계산
	std::vector<int> histogram(256, 0);
	for (int i = 0; i < pixel_count; i++) {
		histogram[gray_pixels[i]]++;
	}

	int total = pixel_count;
	float sum = 0;
	for (int t = 0; t < 256; t++) sum += t * histogram[t];

	float sumB = 0;
	int wB = 0;
	int wF = 0;

	float maxVar = 0;
	int threshold = 0;

	for (int t = 0; t < 256; t++) {
		wB += histogram[t]; // 배경 가중치
		if (wB == 0) continue;

		wF = total - wB; // 전경 가중치
		if (wF == 0) break;

		sumB += (float)(t * histogram[t]);

		float mB = sumB / wB; // 배경 평균
		float mF = (sum - sumB) / wF; // 전경 평균

		// 클래스 간 분산 계산
		float varBetween = (float)wB * (float)wF * (mB - mF) * (mB - mF);

		if (varBetween > maxVar) {
			maxVar = varBetween;
			threshold = t;
		}
	}

	// 3. 계산된 임계값을 사용하여 원본 BGRA 이미지에 이진화 적용
	for (int i = 0; i < pixel_count; ++i) {
		unsigned char value = (gray_pixels[i] > threshold) ? 255 : 0;
		pixels[i * channels + 0] = value; // Blue
		pixels[i * channels + 1] = value; // Green
		pixels[i * channels + 2] = value; // Red
		// Alpha 채널은 그대로 유지
	}
}

void NativeEngine::ImageProcessingEngine::ApplyDilation(unsigned char* pixels, int width, int height) {
	const int channels = 4;
	const int stride = width * channels;
	const int dataSize = stride * height;
	std::vector<unsigned char> temp(dataSize);
	memcpy(temp.data(), pixels, dataSize); // 원본 이미지를 temp에 복사

	// 가장자리를 제외한 픽셀 순회
	for (int y = 1; y < height - 1; y++) {
		for (int x = 1; x < width - 1; x++) {
			int current_idx = y * stride + x * channels;
			unsigned char maxVal = 0;

			// 3x3 커널 순회
			for (int ky = -1; ky <= 1; ky++) {
				for (int kx = -1; kx <= 1; kx++) {
					int neighbor_idx = (y + ky) * stride + (x + kx) * channels;
					// B 채널 값을 기준으로 가장 밝은 값을 찾음
					if (temp[neighbor_idx] > maxVal) {
						maxVal = temp[neighbor_idx];
					}
				}
			}

			// B, G, R 채널을 찾은 최대값으로 설정
			pixels[current_idx + 0] = maxVal;
			pixels[current_idx + 1] = maxVal;
			pixels[current_idx + 2] = maxVal;
		}
	}
}

void NativeEngine::ImageProcessingEngine::ApplyErosion(unsigned char* pixels, int width, int height) {
	const int channels = 4;
	const int stride = width * channels;
	const int dataSize = stride * height;
	std::vector<unsigned char> temp(dataSize);
	memcpy(temp.data(), pixels, dataSize); // 원본 이미지를 temp에 복사

	// 가장자리를 제외한 픽셀 순회
	for (int y = 1; y < height - 1; y++) {
		for (int x = 1; x < width - 1; x++) {
			int current_idx = y * stride + x * channels;
			unsigned char minVal = 255;

			// 3x3 커널 순회
			for (int ky = -1; ky <= 1; ky++) {
				for (int kx = -1; kx <= 1; kx++) {
					int neighbor_idx = (y + ky) * stride + (x + kx) * channels;
					// B 채널 값을 기준으로 가장 어두운 값을 찾음
					if (temp[neighbor_idx] < minVal) {
						minVal = temp[neighbor_idx];
					}
				}
			}

			// B, G, R 채널을 찾은 최소값으로 설정
			pixels[current_idx + 0] = minVal;
			pixels[current_idx + 1] = minVal;
			pixels[current_idx + 2] = minVal;
		}
	}
}
void NativeEngine::ImageProcessingEngine::ApplySobel(unsigned char* pixels, int width, int height) {
	const int channels = 4;
	const int stride = width * channels;
	const int pixel_count = width * height;

	// 1. BGRA 이미지를 그레이스케일 임시 버퍼로 변환합니다.
	std::vector<unsigned char> gray_pixels(pixel_count);
	for (int i = 0; i < pixel_count; ++i) {
		gray_pixels[i] = static_cast<unsigned char>(
			0.114 * pixels[i * channels + 0] + // Blue
			0.587 * pixels[i * channels + 1] + // Green
			0.299 * pixels[i * channels + 2]   // Red
			);
	}

	// 그래디언트와 최종 결과를 저장할 버퍼
	std::vector<unsigned char> result(pixel_count, 0);
	std::vector<double> gradientX(pixel_count, 0);
	std::vector<double> gradientY(pixel_count, 0);

	// 소벨 커널
	int kernelX[3][3] = { {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1} };
	int kernelY[3][3] = { {1, 2, 1}, {0, 0, 0}, {-1, -2, -1} };

	// 2. 그레이스케일 이미지에 컨볼루션 연산을 적용합니다.
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

	// 3. 그래디언트 크기를 계산하여 최종 그레이스케일 결과를 만듭니다.
	for (int i = 0; i < pixel_count; i++) {
		double magnitude = sqrt(pow(gradientX[i], 2) + pow(gradientY[i], 2));
		result[i] = static_cast<unsigned char>(std::clamp(magnitude, 0.0, 255.0));
	}

	// 4. 그레이스케일 결과를 원본 BGRA 버퍼의 B, G, R 채널에 복사합니다.
	for (int i = 0; i < pixel_count; ++i) {
		pixels[i * channels + 0] = result[i]; // Blue
		pixels[i * channels + 1] = result[i]; // Green
		pixels[i * channels + 2] = result[i]; // Red
		// Alpha 채널은 변경하지 않습니다.
	}
}

void NativeEngine::ImageProcessingEngine::ApplyLaplacian(unsigned char* pixels, int width, int height) {
	const int channels = 4;
	const int stride = width * channels;
	const int pixel_count = width * height;

	// 1. BGRA 이미지를 그레이스케일로 변환
	std::vector<unsigned char> gray_pixels(pixel_count);
	for (int i = 0; i < pixel_count; ++i) {
		gray_pixels[i] = static_cast<unsigned char>(
			0.114 * pixels[i * channels + 0] + // Blue
			0.587 * pixels[i * channels + 1] + // Green
			0.299 * pixels[i * channels + 2]   // Red
			);
	}

	// 2. 라플라시안 연산 결과를 저장할 버퍼 (음수 값 포함 가능)
	std::vector<int> laplacian_result(pixel_count, 0);

	// 3. 8-방향 라플라시안 커널을 적용하여 엣지 검출
	//    (더 많은 방향의 엣지를 검출하여 4-방향보다 결과가 부드럽습니다.)
	for (int y = 1; y < height - 1; y++) {
		for (int x = 1; x < width - 1; x++) {
			int current_idx = y * width + x;

			int p1 = gray_pixels[(y - 1) * width + (x - 1)];
			int p2 = gray_pixels[(y - 1) * width + x];
			int p3 = gray_pixels[(y - 1) * width + (x + 1)];
			int p4 = gray_pixels[y * width + (x - 1)];
			int p5_current = gray_pixels[current_idx];
			int p6 = gray_pixels[y * width + (x + 1)];
			int p7 = gray_pixels[(y + 1) * width + (x - 1)];
			int p8 = gray_pixels[(y + 1) * width + x];
			int p9 = gray_pixels[(y + 1) * width + (x + 1)];

			// 8-방향 라플라시안 연산: (주변 8개 픽셀 합) - 8 * 중앙값
			int laplacian_val = (p1 + p2 + p3 + p4 + p6 + p7 + p8 + p9) - 8 * p5_current;

			// 엣지를 시각화하기 위해 절댓값을 취함
			laplacian_result[current_idx] = abs(laplacian_val);
		}
	}

	// 4. 결과 값을 0-255 범위로 정규화하여 가시성을 높임
	int max_val = 0;
	for (int val : laplacian_result) {
		if (val > max_val) {
			max_val = val;
		}
	}
	// 0으로 나누는 오류 방지
	if (max_val == 0) max_val = 1;

	// 5. 정규화된 결과를 원본 BGRA 버퍼에 복사
	for (int i = 0; i < pixel_count; ++i) {
		// (현재 값 / 최댓값) * 255
		unsigned char final_val = static_cast<unsigned char>((laplacian_result[i] * 255) / max_val);
		pixels[i * channels + 0] = final_val; // Blue
		pixels[i * channels + 1] = final_val; // Green
		pixels[i * channels + 2] = final_val; // Red
		// Alpha 채널은 그대로 유지
	}
}


void NativeEngine::ImageProcessingEngine::ApplyTemplateMatch(
	unsigned char* originalPixels, int originalWidth, int originalHeight,
	unsigned char* templatePixels, int templateWidth, int templateHeight,
	int* matchX, int* matchY)
{
	const int channels = 4;

	// 1. 원본 이미지를 그레이스케일로 변환
	int original_pixel_count = originalWidth * originalHeight;
	std::vector<unsigned char> original_gray(original_pixel_count);
	for (int i = 0; i < original_pixel_count; ++i) {
		original_gray[i] = static_cast<unsigned char>(
			(originalPixels[i * channels] + originalPixels[i * channels + 1] + originalPixels[i * channels + 2]) / 3.0
			);
	}

	// 2. 템플릿 이미지를 그레이스케일로 변환
	int template_pixel_count = templateWidth * templateHeight;
	std::vector<unsigned char> template_gray(template_pixel_count);
	for (int i = 0; i < template_pixel_count; ++i) {
		template_gray[i] = static_cast<unsigned char>(
			(templatePixels[i * channels] + templatePixels[i * channels + 1] + templatePixels[i * channels + 2]) / 3.0
			);
	}

	long long minSAD = -1; // SAD(절대차 합계)의 최솟값을 저장할 변수
	*matchX = -1;
	*matchY = -1;

	// 3. 그레이스케일 이미지 위에서 템플릿 매칭 수행
	for (int y = 0; y <= originalHeight - templateHeight; y++) {
		for (int x = 0; x <= originalWidth - templateWidth; x++) {
			long long currentSAD = 0;

			// 템플릿 영역을 순회하며 SAD 계산
			for (int ty = 0; ty < templateHeight; ty++) {
				for (int tx = 0; tx < templateWidth; tx++) {
					int original_idx = (y + ty) * originalWidth + (x + tx);
					int template_idx = ty * templateWidth + tx;

					currentSAD += abs(original_gray[original_idx] - template_gray[template_idx]);
				}
			}

			// SAD 값이 가장 작은 위치를 최적의 매칭 위치로 간주
			if (minSAD == -1 || currentSAD < minSAD) {
				minSAD = currentSAD;
				*matchX = x;
				*matchY = y;
			}
		}
	}
}

void fft1d(vector<complex<double>>& data, bool inverse = false) {
	int n = data.size();
	if (n <= 1) return;

	// 비트 반전 순서로 데이터 재정렬
	for (int i = 1, j = 0; i < n; i++) {
		int bit = n >> 1;
		for (; j & bit; bit >>= 1)
			j ^= bit;
		j ^= bit;
		if (i < j)
			swap(data[i], data[j]);
	}

	// 버터플라이 연산
	for (int len = 2; len <= n; len <<= 1) {
		double ang = 2 * std::numbers::pi / len * (inverse ? -1 : 1);
		complex<double> wlen(cos(ang), sin(ang));
		for (int i = 0; i < n; i += len) {
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
		for (auto& val : data) {
			val /= n;
		}
	}
}

//shift 가 빠짐!
void NativeEngine::ImageProcessingEngine::fftShift() {
	int height = _fftData.size();
	int width = _fftData[0].size();
	int cx = width / 2;
	int cy = height / 2;

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

	// 1. FFT를 위해 이미지 크기를 2의 거듭제곱으로 패딩
	int padWidth = nextPowerOf2(width);
	int padHeight = nextPowerOf2(height);

	_fftData.assign(padHeight, vector<complex<double>>(padWidth));

	// 2. 그레이스케일로 변환하고 패딩된 데이터 구조에 복사 (빈 공간은 0으로 채움)
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

	// 3. 2D FFT 수행 (행 -> 열 순서)
	for (int j = 0; j < padHeight; j++) fft1d(_fftData[j], false);
	for (int i = 0; i < padWidth; i++) {
		vector<complex<double>> col(padHeight);
		for (int j = 0; j < padHeight; j++) col[j] = _fftData[j][i];
		fft1d(col, false);
		for (int j = 0; j < padHeight; j++) _fftData[j][i] = col[j];
	}

	_fftDataBackup = _fftData; // 역변환을 위해 un-shifted 데이터 백업

	// 4. 스펙트럼 시각화를 위해 중앙으로 이동 (FFT Shift)
	fftShift();

	// 5. 로그 스케일링으로 명암 대비 조절
	double max_val = 0.0;
	vector<vector<double>> mag(padHeight, vector<double>(padWidth));
	for (int j = 0; j < padHeight; j++) {
		for (int i = 0; i < padWidth; i++) {
			double val = log(1.0 + abs(_fftData[j][i]));
			mag[j][i] = val;
			if (val > max_val) max_val = val;
		}
	}

	// 6. 0-255로 정규화하여 원본 크기의 버퍼에 결과 저장
	if (max_val == 0) max_val = 1.0;
	int startX = (padWidth - width) / 2;
	int startY = (padHeight - height) / 2;
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

	// 1. 백업해 둔 un-shifted FFT 데이터를 복원
	_fftData = _fftDataBackup;

	// 2. 2D IFFT 수행 (행 -> 열 순서)
	for (int y = 0; y < padHeight; y++) fft1d(_fftData[y], true);
	for (int x = 0; x < padWidth; x++) {
		vector<complex<double>> col(padHeight);
		for (int y = 0; y < padHeight; y++) col[y] = _fftData[y][x];
		fft1d(col, true);
		for (int y = 0; y < padHeight; y++) _fftData[y][x] = col[y];
	}

	// 3. 복원된 데이터를 원본 이미지 크기에 맞게 잘라서 저장
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			double val = _fftData[y][x].real();
			unsigned char gray = static_cast<unsigned char>(std::clamp(round(val), 0.0, 255.0));

			int idx = (y * width + x) * channels;
			pixels[idx + 0] = gray; // Blue
			pixels[idx + 1] = gray; // Green
			pixels[idx + 2] = gray; // Red
			pixels[idx + 3] = 255;  // Alpha (불투명)
		}
	}

	// 4. 사용이 끝난 데이터 초기화
	_fftDataBackup.clear();
	_fftData.clear();

	return true;
}

void NativeEngine::ImageProcessingEngine::ClearFFTData() {
	_fftDataBackup.clear();
	_fftData.clear();
}

bool NativeEngine::ImageProcessingEngine::HasFFTData() {
	return !_fftDataBackup.empty() && !_fftDataBackup[0].empty();
}