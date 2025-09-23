#include <fstream>
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
	}
}

//���� �����Ѵ� (alpha)

void NativeEngine::ImageProcessingEngine::ApplyGaussianBlur(unsigned char* pixels, int width, int height, float sigma) {
	const int channels = 4; // BGRA �����̹Ƿ� 4ä��
	const int stride = width * channels;
	const int dataSize = stride * height;

	// ����þ� Ŀ�� ���
	int kernelRadius = static_cast<int>(ceil(sigma * 3));
	int kernelSize = kernelRadius * 2 + 1;
	std::vector<double> kernel(kernelSize);
	double sum = 0.0;

	for (int i = 0; i < kernelSize; i++) {
		kernel[i] = exp(-0.5 * pow((i - kernelRadius) / sigma, 2.0));
		sum += kernel[i];
	}
	// ����ȭ
	for (int i = 0; i < kernelSize; i++) {
		kernel[i] /= sum;
	}

	std::vector<unsigned char> temp(dataSize);

	//���� �� ����
#pragma omp parallel for (static) private(b, g, r)
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
			temp[out_idx + 3] = pixels[out_idx + 3]; // ���� ä���� �״�� ����
		}
	}

	// ���� �� ����
#pragma omp parallel for (static) private(b, g, r)
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
			// ���� ä�� ó�� �ʿ� ���� (����)
		}
	}
}

void NativeEngine::ImageProcessingEngine::ApplyMedian(unsigned char* pixels, int width, int height, int kernelSize) {
	const int channels = 4; // BGRA ����
	const int stride = width * channels;
	const int dataSize = stride * height;

	// ���� ��� ���� ���� - 4ä��
	std::vector<unsigned char> result(dataSize);

	int kernelHalf = kernelSize / 2;

	// ��ȸ
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			// �̿��� ����
			std::vector<unsigned char> neighborhood_b;
			std::vector<unsigned char> neighborhood_g;
			std::vector<unsigned char> neighborhood_r;

			// Ŀ��
			for (int ky = -kernelHalf; ky <= kernelHalf; ky++) {
				for (int kx = -kernelHalf; kx <= kernelHalf; kx++) {
					// ��� ó��
					int nx = std::clamp(x + kx, 0, width - 1);
					int ny = std::clamp(y + ky, 0, height - 1);

					// 4ä���� ����� �ε��� ���
					int neighbor_idx = ny * stride + nx * channels;

					// �� ä���� ���� �ش� ���Ϳ� �߰�
					neighborhood_b.push_back(pixels[neighbor_idx + 0]);
					neighborhood_g.push_back(pixels[neighbor_idx + 1]);
					neighborhood_r.push_back(pixels[neighbor_idx + 2]);
				}
			}

			// ä�� ���� - �߾Ӱ� ã��
			std::sort(neighborhood_b.begin(), neighborhood_b.end());
			std::sort(neighborhood_g.begin(), neighborhood_g.end());
			std::sort(neighborhood_r.begin(), neighborhood_r.end());

			int current_idx = y * stride + x * channels;

			// �߾Ӱ����� ���� ä���
			size_t median_index = neighborhood_b.size() / 2;
			result[current_idx + 0] = neighborhood_b[median_index]; // B
			result[current_idx + 1] = neighborhood_g[median_index]; // G
			result[current_idx + 2] = neighborhood_r[median_index]; // R
			result[current_idx + 3] = pixels[current_idx + 3]; // Alpha�� ���� �״��
		}
	}

	// ��� ����
	memcpy(pixels, result.data(), dataSize);
}

void NativeEngine::ImageProcessingEngine::ApplyBinarization(unsigned char* pixels, int width, int height) {
	const int channels = 4;
	const int stride = width * channels;
	const int pixelCount = width * height;

	std::vector<unsigned char> gray(pixelCount);
	std::vector<int> histogram(256, 0);

	// RGB �� Grayscale ��ȯ
#pragma omp parallel for
	for (int i = 0; i < pixelCount; ++i) {
		gray[i] = static_cast<unsigned char>(
			0.114 * pixels[i * channels + 0] +  // Blue
			0.587 * pixels[i * channels + 1] +  // Green
			0.299 * pixels[i * channels + 2]    // Red
			);
	}

	// ������׷�
#pragma omp parallel for reduction(+:histogram[:256])
	for (int i = 0; i < pixelCount; i++) {
		histogram[gray[i]]++;
	}

	// ��ü �ȼ� ��� ��
	float totalSum = 0.0f;
	for (int i = 0; i < 256; i++) {
		totalSum += i * histogram[i];
	}

	// �����˰��� ���
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

		// Ŭ���� �� �л�
		float varianceBetween = static_cast<float>(weightForeground) *
			static_cast<float>(weightBackground) *
			(meanForeground - meanBackground) *
			(meanForeground - meanBackground);

		if (varianceBetween > maxVariance) {
			maxVariance = varianceBetween;
			optimalThreshold = t;
		}
	}

	//���� �Ӱ谪���� ����ȭ ó��
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

	// ���� �׷����Ʈ ���� �̹���
#pragma omp parallel for
	for (int i = 0; i < pixelNum; i++) {
		double magnitude = sqrt(pow(gradientX[i], 2) + pow(gradientY[i], 2));
		result[i] = static_cast<unsigned char>(std::clamp(magnitude, 0.0, 255.0));
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
	int max = 0;
	for (int val : laplacianResult) {
		if (val > max) {
			max = val;
		}
	}
	// 0 ������ ����
	if (max == 0) {
		max = 1;
	}

	// ����ȭ �������
#pragma omp parallel for
	for (int i = 0; i < pixelNum; ++i) {
		unsigned char result = static_cast<unsigned char>((laplacianResult[i] * 255) / max);
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

	// ���� �׷��̽����� ��ȯ
	int originalPixelNum = originalWidth * originalHeight;
	std::vector<unsigned char> originalGrayScaled(originalPixelNum);

#pragma omp parallel for
	for (int i = 0; i < originalPixelNum; ++i) {
		originalGrayScaled[i] = static_cast<unsigned char>(
			(originalPixels[i * channels] + originalPixels[i * channels + 1] + originalPixels[i * channels + 2]) / 3.0
			);
	}

	// ���ø��� ��ȯ
	int templatePixelNum = templateWidth * templateHeight;
	std::vector<unsigned char> templateGrayScaled(templatePixelNum);
#pragma omp parallel for
	for (int i = 0; i < templatePixelNum; ++i) {
		templateGrayScaled[i] = static_cast<unsigned char>(
			(templatePixels[i * channels] + templatePixels[i * channels + 1] + templatePixels[i * channels + 2]) / 3.0
			);
	}

	long long min = -1; // ������ ���� �ּڰ� ����
	*matchX = -1;
	*matchY = -1;

	// ���ø� ��Ī ����
#pragma omp parallel for
	for (int y = 0; y <= originalHeight - templateHeight; y++) {
		for (int x = 0; x <= originalWidth - templateWidth; x++) {
			long long currentSAD = 0;

			// ���ø� ���� ��ȸ -> ������ �հ� ���
			for (int ty = 0; ty < templateHeight; ty++) {
				for (int tx = 0; tx < templateWidth; tx++) {
					int originalIdx = (y + ty) * originalWidth + (x + tx);
					int templateIdx = ty * templateWidth + tx;

					currentSAD += abs(originalGrayScaled[originalIdx] - templateGrayScaled[templateIdx]);
				}
			}

			// ������ ���� ���� ���� ��ġ ���
			if (min == -1 || currentSAD < min) {
				min = currentSAD;
				*matchX = x;
				*matchY = y;
			}
		}
	}
}

void fft1d(vector<complex<double>>& data, bool inverse = false) {
	int num = data.size();
	if (num <= 1) return;

	// ��Ʈ ���� ������ ������ ������
#pragma omp parallel for
	for (int i = 1, j = 0; i < num; i++) {
		int bit = num >> 1;
		for (; j & bit; bit >>= 1)
			j ^= bit;
		j ^= bit;
		if (i < j)
			swap(data[i], data[j]);
	}

	// �����ö��� ����
#pragma omp parallel for
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
		for (auto& val : data) {
			val /= num;
		}
	}
}

//shift �� ����!
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

//�е������� �׳� �Լ��� ����
int nextPowerOf2(int n) {
	int p = 1;
	while (p < n) p <<= 1;
	return p;
}

bool NativeEngine::ImageProcessingEngine::ApplyFFT(unsigned char* pixels, int width, int height) {
	const int channels = 4; // BGRA

	// �е��ϱ�
	int padWidth = nextPowerOf2(width);
	int padHeight = nextPowerOf2(height);

	_fftData.assign(padHeight, vector<complex<double>>(padWidth));

	//�׷��̽����� ��ȯ, ���� �е� ����
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

	// �߾����� �̵�
	fftShift();

	//�α� �����ϸ� -> ��� ��� �����ؾ� ��
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

	//����ȭ(0~255)�ؼ� ����
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
			pixels[idx + 3] = 255; // Alpha (������)
		}
	}

	return !_fftData.empty() && !_fftData[0].empty();
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