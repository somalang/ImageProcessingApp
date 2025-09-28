#include <omp.h>
#include <windows.h>
#include <string>
#include "ImageProcessingEngineApp.h"

using namespace std;

void NativeEngine::ImageProcessingEngine::ApplyGrayscale(unsigned char* pixels, int width, int height) {
	// ������ ũ�⸦ Ȯ���Ѵ� -> �Ű������� ����
	// ��� �ȼ��� RGB ���� ���Ѵ�
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

void NativeEngine::ImageProcessingEngine::ApplyGaussianBlur(
	unsigned char* pixels, int width, int height, int radius)
{
	const int channels = 4;
	const int stride = width * channels;
	const int dataSize = stride * height;
	const int kernelSize = (radius * 2) + 1;

	// �޸� �Ҵ��� �� ����
	std::vector<unsigned char> tempBuffer(dataSize);

	// �ݺ� �ּ�ȭ (3�� -> 1��)
	// ---------------------
	// 1. ���� �� (���� ó��)
	// ---------------------
#pragma omp parallel for schedule(static)
	for (int y = 0; y < height; ++y) {
		int sumB = 0, sumG = 0, sumR = 0;
		const int row_offset = y * stride;

		// ù ������ �հ� ��� ����ȭ
		for (int i = -radius; i <= radius; ++i) {
			const int xi = std::clamp(i, 0, width - 1);
			const int idx = row_offset + xi * channels;
			sumB += pixels[idx];
			sumG += pixels[idx + 1];
			sumR += pixels[idx + 2];
		}

		for (int x = 0; x < width; ++x) {
			const int out_idx = row_offset + x * channels;

			// ���� ������ ����ȭ
			tempBuffer[out_idx] = sumB / kernelSize;
			tempBuffer[out_idx + 1] = sumG / kernelSize;
			tempBuffer[out_idx + 2] = sumR / kernelSize;
			tempBuffer[out_idx + 3] = pixels[out_idx + 3];

			// �����̵� ������ ������Ʈ
			const int old_x = std::clamp(x - radius, 0, width - 1);
			const int new_x = std::clamp(x + radius + 1, 0, width - 1);
			const int old_idx = row_offset + old_x * channels;
			const int new_idx = row_offset + new_x * channels;

			sumB += pixels[new_idx] - pixels[old_idx];
			sumG += pixels[new_idx + 1] - pixels[old_idx + 1];
			sumR += pixels[new_idx + 2] - pixels[old_idx + 2];
		}
	}

	// ---------------------
	// 2. ���� �� (���� ó��)
	// ---------------------
#pragma omp parallel for schedule(static)
	for (int x = 0; x < width; ++x) {
		int sumB = 0, sumG = 0, sumR = 0;
		const int col_offset = x * channels;

		// ù ������ �հ� ���
		for (int i = -radius; i <= radius; ++i) {
			const int yi = std::clamp(i, 0, height - 1);
			const int idx = yi * stride + col_offset;
			sumB += tempBuffer[idx];
			sumG += tempBuffer[idx + 1];
			sumR += tempBuffer[idx + 2];
		}

		for (int y = 0; y < height; ++y) {
			const int out_idx = y * stride + col_offset;

			pixels[out_idx] = sumB / kernelSize;
			pixels[out_idx + 1] = sumG / kernelSize;
			pixels[out_idx + 2] = sumR / kernelSize;

			// �����̵� ������ ������Ʈ
			const int old_y = std::clamp(y - radius, 0, height - 1);
			const int new_y = std::clamp(y + radius + 1, 0, height - 1);
			const int old_idx = old_y * stride + col_offset;
			const int new_idx = new_y * stride + col_offset;

			sumB += tempBuffer[new_idx] - tempBuffer[old_idx];
			sumG += tempBuffer[new_idx + 1] - tempBuffer[old_idx + 1];
			sumR += tempBuffer[new_idx + 2] - tempBuffer[old_idx + 2];
		}
	}
}
void NativeEngine::ImageProcessingEngine::ApplyMedian(
	unsigned char* pixels, int width, int height, int kernelSize)
{
	const int channels = 4;
	const int stride = width * channels;
	const int dataSize = stride * height;

	std::vector<unsigned char> result(dataSize);
	int kernelHalf = kernelSize / 2;
	int kernelArea = kernelSize * kernelSize;
	const int medianIndex = kernelArea / 2;

#pragma omp parallel for schedule(static)
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int histB[256] = { 0 }, histG[256] = { 0 }, histR[256] = { 0 };

			// Ŀ�� �� �ȼ� ī����
			for (int ky = -kernelHalf; ky <= kernelHalf; ky++) {
				for (int kx = -kernelHalf; kx <= kernelHalf; kx++) {
					int nx = std::clamp(x + kx, 0, width - 1);
					int ny = std::clamp(y + ky, 0, height - 1);
					int idx = ny * stride + nx * channels;

					histB[pixels[idx + 0]]++;
					histG[pixels[idx + 1]]++;
					histR[pixels[idx + 2]]++;
				}
			}

			// �߾Ӱ� ã�� (BGR ����)
			auto getMedian = [&](int* hist) {
				int count = 0;
				for (int i = 0; i < 256; i++) {
					count += hist[i];
					if (count > medianIndex) return i;
				}
				return 0;
				};

			int outIdx = y * stride + x * channels;
			result[outIdx + 0] = (unsigned char)getMedian(histB);
			result[outIdx + 1] = (unsigned char)getMedian(histG);
			result[outIdx + 2] = (unsigned char)getMedian(histR);
			result[outIdx + 3] = pixels[outIdx + 3]; // ���� ����
		}
	}

	memcpy(pixels, result.data(), dataSize);
}


void NativeEngine::ImageProcessingEngine::ApplyBinarization(unsigned char* pixels, int width, int height) {
	const int channels = 4;
	const int stride = width * channels;
	const int pixelCount = width * height;

	std::vector<unsigned char> gray(pixelCount);
	std::vector<int> histogram(256, 0);

	// 1. RGB �� Grayscale ��ȯ (���� ó��)
#pragma omp parallel for
	for (int i = 0; i < pixelCount; ++i) {
		gray[i] = static_cast<unsigned char>(
			0.114 * pixels[i * channels + 0] +  // Blue
			0.587 * pixels[i * channels + 1] +  // Green
			0.299 * pixels[i * channels + 2]    // Red
			);
	}

	// ������׷� ���
#pragma omp parallel
	{
		// �����庰 ���� ������׷��� ����
		std::vector<int> local_histogram(256, 0);

		// ������ -> ���� ������׷��� ����
#pragma omp for nowait
		for (int i = 0; i < pixelCount; i++) {
			local_histogram[gray[i]]++;
		}

		// ���� -> ��ü ������׷�
#pragma omp critical
		for (int i = 0; i < 256; i++) {
			histogram[i] += local_histogram[i];
		}
	}

	// ���� �Ӱ谪 ���
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

	// ���� �Ӱ谪
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
	memcpy(temp.data(), pixels, dataSize); // ���� �̹����� temp�� ����

	// �����ڸ��� ������ �ȼ� ��ȸ
#pragma omp parallel for
	for (int y = 1; y < height - 1; y++) {
		for (int x = 1; x < width - 1; x++) {
			int current = y * stride + x * channels;
			unsigned char max = 0;

			// 3x3 Ŀ�� ��ȸ
			for (int ky = -1; ky <= 1; ky++) {
				for (int kx = -1; kx <= 1; kx++) {
					int neighborIdx = (y + ky) * stride + (x + kx) * channels;
					// B ä�� ���� ���� ���� ��
					if (temp[neighborIdx] > max) {
						max = temp[neighborIdx];
					}
				}
			}

			//�ִ밪 ����
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

	// �����ڸ� ���� �ȼ� ��ȸ
#pragma omp parallel for
	for (int y = 1; y < height - 1; y++) {
		for (int x = 1; x < width - 1; x++) {
			int current_idx = y * stride + x * channels;
			unsigned char min = 255;

			// 3x3 Ŀ�� ��ȸ
			for (int ky = -1; ky <= 1; ky++) {
				for (int kx = -1; kx <= 1; kx++) {
					int neighbor_idx = (y + ky) * stride + (x + kx) * channels;
					// B ���� ���� ��ο
					if (temp[neighbor_idx] < min) {
						min = temp[neighbor_idx];
					}
				}
			}

			// �ּҰ� ����
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

	// �׷��̽����� �����ؼ� �ӽù��� �ֱ�
	std::vector<unsigned char> gray_pixels(pixelNum);
#pragma omp parallel for
	for (int i = 0; i < pixelNum; ++i) {
		gray_pixels[i] = static_cast<unsigned char>(
			0.114 * pixels[i * channels + 0] + // Blue
			0.587 * pixels[i * channels + 1] + // Green
			0.299 * pixels[i * channels + 2]   // Red
			);
	}

	// �׷����Ʈ�� ���� ����� ������ ����
	std::vector<unsigned char> result(pixelNum, 0);
	std::vector<double> gradientX(pixelNum, 0);
	std::vector<double> gradientY(pixelNum, 0);

	// �Һ� Ŀ��
	int kernelX[3][3] = { {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1} };
	int kernelY[3][3] = { {1, 2, 1}, {0, 0, 0}, {-1, -2, -1} };

	// ������� ����
#pragma omp parallel for
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
			double magnitude = sqrt(pow(sumX, 2) + pow(sumY, 2));
			result[y * width + x] = static_cast<unsigned char>(std::clamp(magnitude, 0.0, 255.0));
		}
	}

	// ������ ����
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

	// �׷��̽����� ��ȯ
	std::vector<unsigned char> temp(pixelNum);

#pragma omp parallel for
	for (int i = 0; i < pixelNum; ++i) {
		temp[i] = static_cast<unsigned char>(
			0.114 * pixels[i * channels + 0] + // Blue
			0.587 * pixels[i * channels + 1] + // Green
			0.299 * pixels[i * channels + 2]   // Red
			);
	}

	// ���ö� ���� ��� ����
	std::vector<int> laplacianResult(pixelNum, 0);

	// 8���� ���
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

			// 8�� �ȼ� �� - 8 * �߾Ӱ�
			int laplacianValue = (upLeft + up + upRight + left + right + downLeft + down + downRight) - 8 * current;

			// ���� -> ���� �ð�ȭ
			laplacianResult[current_idx] = abs(laplacianValue);
		}
	}

	// ����ȭ
	int max_val = 0;
#pragma omp parallel for reduction(max:max_val)
	for (int i = 0; i < pixelNum; i++) {
		if (laplacianResult[i] > max_val) {
			max_val = laplacianResult[i];
		}
	}

	if (max_val == 0) max_val = 1;
	// 0 ������ ����
	if (max_val == 0) {
		max_val = 1;
	}

	// ����ȭ �������
#pragma omp parallel for
	for (int i = 0; i < pixelNum; ++i) {
		unsigned char result = static_cast<unsigned char>((laplacianResult[i] * 255) / max_val);
		pixels[i * channels + 0] = result; // Blue
		pixels[i * channels + 1] = result; // Green
		pixels[i * channels + 2] = result; // Red
		// Alpha ä���� �״�� ����
	}
}

void NativeEngine::ImageProcessingEngine::ApplyTemplateMatch(
	unsigned char* originalPixels, int originalWidth, int originalHeight,
	unsigned char* templatePixels, int templateWidth, int templateHeight,
	int* matchX, int* matchY)
{
	const int channels = 4;
	const int originalPixelNum = originalWidth * originalHeight;
	const int templatePixelNum = templateWidth * templateHeight;

	// �޸� �Ҵ��� �� ����
	std::vector<unsigned char> originalGray(originalPixelNum);
	std::vector<unsigned char> templateGray(templatePixelNum);

	// �׷��̽����� ��ȯ ����ȭ (���� ����)
#pragma omp parallel for schedule(static)
	for (int i = 0; i < originalPixelNum; ++i) {
		const int base = i * channels;
		const unsigned int sum = originalPixels[base] + originalPixels[base + 1] + originalPixels[base + 2];
		originalGray[i] = static_cast<unsigned char>(sum / 3);
	}

#pragma omp parallel for schedule(static)
	for (int i = 0; i < templatePixelNum; ++i) {
		const int base = i * channels;
		const unsigned int sum = templatePixels[base] + templatePixels[base + 1] + templatePixels[base + 2];
		templateGray[i] = static_cast<unsigned char>(sum / 3);
	}

	// �˻� ����
	const int searchHeight = originalHeight - templateHeight + 1;
	const int searchWidth = originalWidth - templateWidth + 1;
	const int totalSearchPoints = searchHeight * searchWidth;

	// ����� ������ ����ü �迭
	struct MatchResult {
		long long sad;
		int x, y;
	};

	std::vector<MatchResult> results(totalSearchPoints);

	// ���ķ� ��� ��ġ���� SAD ���
#pragma omp parallel for schedule(static)
	for (int idx = 0; idx < totalSearchPoints; idx++) {
		const int y = idx / searchWidth;
		const int x = idx % searchWidth;

		long long currentSAD = 0;

		// ĳ�� ģȭ���� ������ ���ø� ��Ī
		for (int ty = 0; ty < templateHeight; ty++) {
			const int origRowStart = (y + ty) * originalWidth + x;
			const int tempRowStart = ty * templateWidth;

			// �� ������ ó���Ͽ� ĳ�� ȿ���� ����
			for (int tx = 0; tx < templateWidth; tx++) {
				const int diff = static_cast<int>(originalGray[origRowStart + tx]) -
					static_cast<int>(templateGray[tempRowStart + tx]);
				currentSAD += (diff < 0) ? -diff : diff;  // abs() ��� ���Ǻ� ����
			}
		}

		results[idx] = { currentSAD, x, y };
	}

	// �ּҰ� ã�� (���������� ó�� - reduction ���)
	MatchResult bestMatch = results[0];
	for (int i = 1; i < totalSearchPoints; i++) {
		if (results[i].sad < bestMatch.sad) {
			bestMatch = results[i];
		}
	}

	*matchX = bestMatch.x;
	*matchY = bestMatch.y;
}

void fft1d(vector<complex<double>>& data, bool inverse = false) {
	int num = data.size();
	if (num <= 1) return;

	// ��Ʈ ���� ������ ������ ������
	for (int i = 1, j = 0; i < num; i++) {
		int bit = num >> 1;
		for (; j & bit; bit >>= 1)
			j ^= bit;
		j ^= bit;
		if (i < j)
			swap(data[i], data[j]);
	}

	// �����ö��� ����
	for (int len = 2; len <= num; len <<= 1) {
		double ang = 2 * std::numbers::pi / len * (inverse ? -1 : 1);
		complex<double> wlen(cos(ang), sin(ang));
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

	// IFFT�� ��� ũ�� ����
	if (inverse) {
		for (int i = 0; i < num; i++) {
			data[i] /= num;
		}
	}
}

//shift �� ����!
void NativeEngine::ImageProcessingEngine::fftShift() {
	int height = _fftData.size();
	int width = _fftData[0].size();
	int cx = width / 2;
	int cy = height / 2;

#pragma omp parallel for collapse(2) schedule(static)
	for (int y = 0; y < cy; y++) {
		for (int x = 0; x < cx; x++) {
			auto tmp = _fftData[y][x];
			_fftData[y][x] = _fftData[y + cy][x + cx];
			_fftData[y + cy][x + cx] = tmp;

			tmp = _fftData[y + cy][x];
			_fftData[y + cy][x] = _fftData[y][x + cx];
			_fftData[y][x + cx] = tmp;
		}
	}
}

//�е������� �׳� �Լ��� ����
int nextPowerOf2(int n) {
	int p = 1;
	while (p < n) p <<= 1;
	return p;
}

bool NativeEngine::ImageProcessingEngine::ApplyFFT(unsigned char* pixels, int width, int height) {
	const int channels = 4;

	// �е� ũ�� ���
	int padWidth = nextPowerOf2(width);
	int padHeight = nextPowerOf2(height);

	_fftData.assign(padHeight, vector<complex<double>>(padWidth));

	// �׷��̽����� ��ȯ�� ���� �е��� �� ���� ó��
#pragma omp parallel for schedule(static)
	for (int j = 0; j < padHeight; j++) {
		for (int i = 0; i < padWidth; i++) {
			if (j < height && i < width) {
				const int idx = (j * width + i) * channels;
				// ���� �������� ����ȭ
				const int weighted_sum = 114 * pixels[idx] + 587 * pixels[idx + 1] + 299 * pixels[idx + 2];
				_fftData[j][i] = complex<double>(weighted_sum / 1000.0, 0.0);
			}
			else {
				_fftData[j][i] = complex<double>(0.0, 0.0);
			}
		}
	}

	// 2D FFT
#pragma omp parallel for schedule(static)
	for (int j = 0; j < padHeight; j++) {
		fft1d(_fftData[j], false);
	}

#pragma omp parallel for schedule(static)
	for (int i = 0; i < padWidth; i++) {
		vector<complex<double>> col(padHeight);
		for (int j = 0; j < padHeight; j++) {
			col[j] = _fftData[j][i];
		}
		fft1d(col, false);
		for (int j = 0; j < padHeight; j++) {
			_fftData[j][i] = col[j];
		}
	}

	_fftDataBackup = _fftData;
	fftShift();

	// ��� �̹��� ���� ����ȭ
	double max_val = 0.0;
	vector<vector<double>> mag(padHeight, vector<double>(padWidth));

#pragma omp parallel for schedule(static)
	for (int j = 0; j < padHeight; j++) {
		for (int i = 0; i < padWidth; i++) {
			mag[j][i] = std::log1p(std::abs(_fftData[j][i]));
		}
	}

	// �ִ밪 ã��
#pragma omp parallel for reduction(max:max_val)
	for (int j = 0; j < padHeight; j++) {
		for (int i = 0; i < padWidth; i++) {
			if (mag[j][i] > max_val) max_val = mag[j][i];
		}
	}

	if (max_val == 0) max_val = 1.0;

	const int startX = (padWidth - width) / 2;
	const int startY = (padHeight - height) / 2;
	const double inv_max = 255.0 / max_val;

#pragma omp parallel for schedule(static)
	for (int j = 0; j < height; j++) {
		for (int x = 0; x < width; x++) {
			const unsigned char v = static_cast<unsigned char>(mag[j + startY][x + startX] * inv_max);
			const int idx = (j * width + x) * channels;
			pixels[idx] = pixels[idx + 1] = pixels[idx + 2] = v;
			pixels[idx + 3] = 255;
		}
	}

	return true;
}

bool NativeEngine::ImageProcessingEngine::ApplyIFFT(unsigned char* pixels, int width, int height) {
	if (_fftDataBackup.empty()) return false;

	const int channels = 4;
	const int padHeight = _fftDataBackup.size();
	const int padWidth = _fftDataBackup[0].size();

	// �����ص� ������ ��� (shift ��)
	_fftData = _fftDataBackup;

	// 2D IFFT (���� -> ����)
#pragma omp parallel for
	for (int y = 0; y < padHeight; y++) fft1d(_fftData[y], true);
#pragma omp parallel for
	for (int x = 0; x < padWidth; x++) {
		vector<complex<double>> col(padHeight);
		for (int y = 0; y < padHeight; y++) col[y] = _fftData[y][x];
		fft1d(col, true);
		for (int y = 0; y < padHeight; y++) _fftData[y][x] = col[y];
	}

	// ���� �̹��� ũ�⿡ �°� ���� �̹��� �߶� ����
#pragma omp parallel for
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			double val = _fftData[y][x].real();
			unsigned char gray = static_cast<unsigned char>(std::clamp(round(val), 0.0, 255.0));

			//BGRA ó���ϱ�
			int idx = (y * width + x) * channels;
			pixels[idx + 0] = gray;
			pixels[idx + 1] = gray;
			pixels[idx + 2] = gray;
			pixels[idx + 3] = 255;
		}
	}

	//��� ���� �� �ʱ�ȭ�ϱ�
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

