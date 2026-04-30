// OpenCVApplication.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "common.h"
#include <opencv2/core/utils/logger.hpp>

wchar_t* projectPath;

struct Data {
	int area;
	int min_r, max_r;
	int min_c, max_c;
	int perimeter;
};

void computeHSV(const Mat& src, Mat& Hnorm, Mat& Snorm, Mat& Vnorm) {
	int height = src.rows;
	int width = src.cols;

	Hnorm = Mat(height, width, CV_8UC1);
	Snorm = Mat(height, width, CV_8UC1);
	Vnorm = Mat(height, width, CV_8UC1);

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			Vec3b p = src.at<Vec3b>(i, j);
			float b = (float)p[0] / 255;
			float g = (float)p[1] / 255;
			float r = (float)p[2] / 255;

			float M = max(r, max(g, b));
			float m = min(r, min(g, b));
			float C = M - m;

			float V = M;
			float S = 0.0f;
			float H = 0.0f;

			if (V != 0) S = C / V;
			if (C != 0) {
				if (M == r) H = 60 * (g - b) / C;
				if (M == g) H = 120 + 60 * (b - r) / C;
				if (M == b) H = 240 + 60 * (r - g) / C;
			}
			if (H < 0) H += 360;

			Hnorm.at<uchar>(i, j) = (uchar)(H * 255 / 360);
			Snorm.at<uchar>(i, j) = (uchar)(S * 255);
			Vnorm.at<uchar>(i, j) = (uchar)(V * 255);
		}
	}
}

void HSV() {
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		Mat src = imread(fname, IMREAD_COLOR);

		resize(src, src, Size(800, 600)); 

		Mat Hnorm, Snorm, Vnorm;
		computeHSV(src, Hnorm, Snorm, Vnorm); 

		imshow("input image", src);
		imshow("hue image", Hnorm);
		imshow("saturation image", Snorm);
		imshow("value image", Vnorm);
		waitKey(0);
	}
}


Mat dilation(Mat src) {
	int height = src.rows;
	int width = src.cols;

	Mat dst(height, width, CV_8UC1, Scalar(0));

	int dr[] = { 0, -1, 1, 0, 0 };
	int dc[] = { 0, 0, 0, -1, 1 };

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			uchar pixel = src.at<uchar>(i, j);
			if (pixel == 255) { 
				for (int k = 0; k < 5; k++) {
					int ni = i + dr[k];
					int nj = j + dc[k];

					if (ni >= 0 && ni < height && nj >= 0 && nj < width) {
						dst.at<uchar>(ni, nj) = 255; 
					}
				}
			}
		}
	}
	return dst;
}

Mat erosion(Mat src) {
	int height = src.rows;
	int width = src.cols;

	Mat dst(height, width, CV_8UC1, Scalar(0));
	int dr[] = { 0, -1, 1, 0, 0 };
	int dc[] = { 0, 0, 0, -1, 1 };

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			bool ok = true;
			for (int k = 0; k < 5; k++) {
				int ni = i + dr[k];
				int nj = j + dc[k];

				if (ni >= 0 && ni < height && nj >= 0 && nj < width) {
					if (src.at<uchar>(ni, nj) == 0) {
						ok = false;
						break;
					}
				}
			}
			if (ok) {
				dst.at<uchar>(i, j) = 255; 
			}
		}
	}
	return dst;
}


void detectSkin(bool ok) {
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		Mat src = imread(fname, IMREAD_COLOR);
		if (src.empty()) continue;

		resize(src, src, Size(800, 600));
		Mat blurredSrc;
		GaussianBlur(src, blurredSrc, Size(5, 5), 0);

		Mat Hnorm, Snorm, Vnorm;
		computeHSV(blurredSrc, Hnorm, Snorm, Vnorm);

		int height = src.rows;
		int width = src.cols;
		Mat skinMask (height, width, CV_8UC1, Scalar(0));

		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				uchar H_val = Hnorm.at<uchar>(i, j);
				uchar S_val = Snorm.at<uchar>(i, j);
				uchar V_val = Vnorm.at<uchar>(i, j);

				bool isSkinHue = (H_val <= 25 || H_val >= 237);
				bool isSkinSat = (S_val >= 51 && S_val <= 204);
				bool isSkinVal = (V_val >= 130);

				if (isSkinHue && isSkinSat && isSkinVal) {
					skinMask.at<uchar>(i, j) = 255;
				}
			}
		}

		Mat openedMask = erosion(skinMask);
		openedMask = dilation(openedMask);

		Mat finalCleanMask = dilation(openedMask);
		finalCleanMask = erosion(finalCleanMask);

		if (ok) {
			FindFaceAndDrawBox(src, finalCleanMask);
		}
		else {
			imshow("Original Image", src);
			imshow("Raw Skin Mask", skinMask);
			imshow("Clean Skin Mask", finalCleanMask);
		}
		waitKey(0);

	}
}
void FindFaceAndDrawBox(Mat src, Mat skin) {
	int height = skin.rows;
	int width = skin.cols;

	int di[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };
	int dj[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };

	int label = 0;
	Mat labels(height, width, CV_32SC1, Scalar(0));

	std::vector<Data> shapes;
	shapes.push_back({ 0, 0, 0, 0, 0, 0 });

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			if (skin.at<uchar>(i, j) == 255 && labels.at<int>(i, j) == 0) {
				label++;

				Data currentShape = { 0, i, i, j, j, 0 };

				std::queue<Point> q;
				q.push(Point(j, i));
				labels.at<int>(i, j) = label;

				while (!q.empty()) {
					Point p = q.front();
					q.pop();

					currentShape.area++;
					if (p.y < currentShape.min_r) currentShape.min_r = p.y;
					if (p.y > currentShape.max_r) currentShape.max_r = p.y;
					if (p.x < currentShape.min_c) currentShape.min_c = p.x;
					if (p.x > currentShape.max_c) currentShape.max_c = p.x;

					bool isEdgePixel = false; 

					for (int k = 0; k < 8; k++) {
						int ni = p.y + di[k];
						int nj = p.x + dj[k];

						if (ni < 0 || ni >= height || nj < 0 || nj >= width) {
							isEdgePixel = true;
							continue;
						}
						if (skin.at<uchar>(ni, nj) == 0) {
							isEdgePixel = true;
						}
						if (skin.at<uchar>(ni, nj) == 255 && labels.at<int>(ni, nj) == 0) {
							labels.at<int>(ni, nj) = label;
							q.push(Point(nj, ni));
						}
					}
					if (isEdgePixel) {
						currentShape.perimeter++;
					}
				}
				shapes.push_back(currentShape); 
			}
		}
	}

	int goodFace = -1;
	int highestY = height; 

	for (int i = 1; i <= label; i++) {
		if (shapes[i].area > 5000) {

			int boxWidth = shapes[i].max_c - shapes[i].min_c;
			int boxHeight = shapes[i].max_r - shapes[i].min_r;
			if (boxWidth == 0 || boxHeight == 0) continue;

			float aspectRatio = (float)boxWidth / (float)boxHeight;

			int boundingBoxArea = boxWidth * boxHeight;
			float fillRatio = (float)shapes[i].area / (float)boundingBoxArea;

			bool isFaceProportions = (aspectRatio > 0.4f && aspectRatio < 1.5f);

			bool isOvalShape = (fillRatio > 0.45f && fillRatio < 0.85f);

			if (isFaceProportions && isOvalShape) {
				if (shapes[i].min_r < highestY) {
					highestY = shapes[i].min_r;
					goodFace = i;
				}
			}


		}
	}

	Mat res = src.clone();

	if (goodFace != -1) {
		Data face = shapes[goodFace];

		int boxWidth = face.max_c - face.min_c;
		int boxHeight = face.max_r - face.min_r;

		int final_min_r = face.min_r;
		int final_max_r = face.max_r;
		int final_min_c = face.min_c;
		int final_max_c = face.max_c;

		if (final_min_r < 0) final_min_r = 0;
		if (final_max_r >= height) final_max_r = height - 1;
		if (final_min_c < 0) final_min_c = 0;
		if (final_max_c >= width) final_max_c = width - 1;

		rectangle(res, Point(final_min_c, final_min_r), Point(final_max_c, final_max_r), Scalar(0, 0, 255), 3); 
	}
	else {
		std::cout << "No face detected in this image.\n";
	}

	imshow("Face Detection Result", res);
}

int main()
{
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_FATAL);
	projectPath = _wgetcwd(0, 0);

	int op;
	do
	{
		system("cls");
		destroyAllWindows();
		printf("Face Detection Project\n");
		printf("\n");;
		printf(" 1 - HSV Operation\n");
		printf(" 2 - Skin Detection\n");
		printf(" 3 - Face Detection\n");
		printf(" 0 - Exit\n\n");
		printf("Option: ");
		scanf("%d", &op);
		switch (op)
		{
		case 1:
			HSV();
			break;
		case 2:
			detectSkin(false);
			break;
		case 3:
			detectSkin(true);
			break;
		}
	} while (op != 0);
	return 0;
}
