
#include "utiils.h"

Pix* mat8ToPix(cv::Mat* mat8)
{
    Pix* pixd = pixCreate(mat8->size().width, mat8->size().height, 8);
    for (int y = 0; y < mat8->rows; y++) {
        for (int x = 0; x < mat8->cols; x++) {
            pixSetPixel(pixd, x, y, (l_uint32)mat8->at<uchar>(y, x));
        }
    }
    return pixd;
}

Pix* mat24ToPix(cv::Mat* mat24)
{
    Pix* pixd = pixCreate(mat24->size().width, mat24->size().height, 32);
    for (int y = 0; y < mat24->rows; y++) {
        for (int x = 0; x < mat24->cols; x++) {
            pixSetRGBPixel(
                pixd,
                x,
                y,
                (l_uint32)mat24->at<uchar>(y, x + 0),
                (l_uint32)mat24->at<uchar>(y, x + 1),
                (l_uint32)mat24->at<uchar>(y, x + 2)
            );
        }
    }
    return pixd;
}

cv::Mat pix8ToMat(Pix* pix8)
{
    cv::Mat mat(cv::Size(pix8->w, pix8->h), CV_8UC1);
    uint32_t* line = pix8->data;
    for (uint32_t y = 0; y < pix8->h; ++y) {
        for (uint32_t x = 0; x < pix8->w; ++x) {
            mat.at<uchar>(y, x) = GET_DATA_BYTE(line, x);
        }
        line += pix8->wpl;
    }
    return mat;
}

cv::Mat pix24ToMat(Pix* pix24)
{
    cv::Mat mat(cv::Size(pix24->w, pix24->h), CV_8UC3);
    uint32_t* line = pix24->data;
    for (uint32_t y = 0; y < pix24->h; ++y) {
        for (uint32_t x = 0; x < pix24->w; ++x) {
            mat.at<uchar>(y, x, 0) = GET_DATA_BYTE( line, x + 0 );
            mat.at<uchar>(y, x, 1) = GET_DATA_BYTE( line, x + 1 );
            mat.at<uchar>(y, x, 2) = GET_DATA_BYTE( line, x + 2 );
        }
        line += pix24->wpl;
    }
    return mat;
}

auto convertCvMatToBMP(cv::Mat frame) -> HBITMAP
{
    auto convertOpenCVBitDepthToBits = [](const int32_t value)
    {
        auto regular = 0u;

        switch (value)
        {
        case CV_8U:
        case CV_8S:
            regular = 8u;
            break;

        case CV_16U:
        case CV_16S:
            regular = 16u;
            break;

        case CV_32S:
        case CV_32F:
            regular = 32u;
            break;

        case CV_64F:
            regular = 64u;
            break;

        default:
            regular = 0u;
            break;
        }

        return regular;
    };

    auto imageSize = frame.size();
    assert(imageSize.width && "invalid size provided by frame");
    assert(imageSize.height && "invalid size provided by frame");

    if (imageSize.width && imageSize.height)
    {
        auto headerInfo = BITMAPINFOHEADER{};
        ZeroMemory(&headerInfo, sizeof(headerInfo));

        headerInfo.biSize = sizeof(headerInfo);
        headerInfo.biWidth = imageSize.width;
        headerInfo.biHeight = -(imageSize.height); // negative otherwise it will be upsidedown
        headerInfo.biPlanes = 1;// must be set to 1 as per documentation frame.channels();

        const auto bits = convertOpenCVBitDepthToBits(frame.depth());
        headerInfo.biBitCount = frame.channels() * bits;

        auto bitmapInfo = BITMAPINFO{};
        ZeroMemory(&bitmapInfo, sizeof(bitmapInfo));

        bitmapInfo.bmiHeader = headerInfo;
        bitmapInfo.bmiColors->rgbBlue = 0;
        bitmapInfo.bmiColors->rgbGreen = 0;
        bitmapInfo.bmiColors->rgbRed = 0;
        bitmapInfo.bmiColors->rgbReserved = 0;

        auto dc = GetDC(nullptr);
        assert(dc != nullptr && "Failure to get DC");
        auto bmp = CreateDIBitmap(dc,
            &headerInfo,
            CBM_INIT,
            frame.data,
            &bitmapInfo,
            DIB_RGB_COLORS);
        assert(bmp != nullptr && "Failure creating bitmap from captured frame");

        return bmp;
    }
    else
    {
        return nullptr;
    }
}
