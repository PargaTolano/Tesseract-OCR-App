#pragma once

#include <opencv2/opencv.hpp>
#include <leptonica/allheaders.h>
#include <Windows.h>
#include <gdiplus.h>

Pix* mat8ToPix(cv::Mat* mat8);

Pix* mat24ToPix(cv::Mat* mat24);

cv::Mat pix8ToMat(Pix* pix8);

cv::Mat pix24ToMat(Pix* pix24);

Gdiplus::Bitmap* pixToGdiplusBitmap(Pix* image, Gdiplus::PixelFormat pixelFormat);

auto convertCvMatToBMP(cv::Mat frame) -> HBITMAP;