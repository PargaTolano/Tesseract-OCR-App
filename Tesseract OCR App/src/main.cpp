//definir __DEBUGGING__ en caso de querer usar impresiones en la consola
//#define __DEBUGGING__

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <opencv2/opencv.hpp>

#include <Windows.h>
#include <commdlg.h>

#include <future>
#include <vector>
#include <queue>
#include <regex>

#include "Encoder/BitmapEncoder.h"

#include "utils/utiils.h"

#include "../resource.h"

#define DEFAULT_CAMERA 1
#define CAPTURE_WINDOW_NAME "Captura de Video"

tesseract::TessBaseAPI* g_tesseractApi;

cv::VideoCapture    g_videoCapture;

std::queue<cv::Mat> g_frameQueue;

std::atomic_bool    g_isCameraOpen( false );

std::thread         g_thCapture, g_thImageProcessing;

Parga_PI::BitmapEncoder* g_bitmapEncoder;

HWND                g_hMainWindow,  g_hImageControl, g_hTextControl;

HBITMAP             g_hPlaceholderBitmap;

HINSTANCE           g_hInstance;

bool                g_running;

#define resultText( text ) SetWindowTextA( g_hTextControl, ( text ) )

#define setBitmap( hImageControl, hBitmap ) SendMessage( ( hImageControl ) , STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)( hBitmap ) )

LRESULT CALLBACK windowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

void captureCamera();

void processImages();

Gdiplus::Bitmap* pixToGdiplusBitmap(Pix* image, Gdiplus::PixelFormat pixelFormat) {

    l_int32 w = pixGetWidth(image),
        h = pixGetHeight(image),
        d = pixGetDepth(image),
        stride = d * w;

    BYTE* data0 = (BYTE*)pixGetData(image);

#ifdef __DEBUGGING__

    printf("%%%%%%%%%%%%%%%%%%%%%%%%%%%\ndepth: %d; r: %d; g: %d; b: %d;\n%%%%%%%%%%%%%%%%%%%%%%%%%%%\n", d, data0[0], data0[1], data0[2]);

#endif

    return new Gdiplus::Bitmap(w, h, stride, pixelFormat, data0);
}

bool initCamera( int i ) {
    if ( !g_videoCapture.open( i ) )
        return false;

    g_isCameraOpen.exchange( true );

    if ( g_thCapture.joinable() )
        g_thCapture.join();

    if ( g_thImageProcessing.joinable() )
        g_thImageProcessing.join();

    g_thCapture = std::thread( captureCamera );
    g_thImageProcessing = std::thread( processImages );
    
    return true;
}

void stopCamera() {

    setBitmap( g_hImageControl, g_hPlaceholderBitmap );

    g_isCameraOpen.exchange( false );

    if( g_thCapture.joinable() )
        g_thCapture.join();

    if (g_thCapture.joinable())
        g_thImageProcessing.join();

#ifdef __DEBUGGING__
    cv::destroyWindow( CAPTURE_WINDOW_NAME );
#endif

    g_videoCapture.release();
    
    while (g_frameQueue.size() > 0)
        g_frameQueue.pop();
}

void captureCamera() {
    cv::Mat frame;
    while (g_isCameraOpen) {

        g_videoCapture >> frame;

        if (frame.empty())
            continue;

        g_frameQueue.push(frame);
    }
}

const char* textProcess( cv::Mat frame ) {

    if ( frame.empty() )
        return 0;

    const char* outText;

    g_tesseractApi->SetImage(frame.data, frame.cols, frame.rows, 3, frame.step);
    outText = g_tesseractApi->GetUTF8Text();

#ifdef __DEBUGGING__

    if (!outText)
        printf("Null outText");

#endif

    std::regex r("\n");

    std::string str = std::regex_replace(outText, r, "\r\n");

    return str.c_str();
}

HBITMAP winAPIBitmapProcess( cv::Mat frame ) {

    if (frame.empty())
        return 0;

    HBITMAP hBmp = convertCvMatToBMP( frame );


#ifdef __DEBUGGING__
        
    if (!hBmp)
        printf("Null HBITMAP");

#endif 

    return hBmp;
}

void processImages() {
    while (g_isCameraOpen) {

        if (g_frameQueue.empty())
            continue;

        cv::Mat frame = std::move( g_frameQueue.front() );
        g_frameQueue.pop();

        std::future<const char*> futTextProccess = std::async( textProcess, frame );
        std::future<HBITMAP> futImageProccess = std::async( winAPIBitmapProcess, frame );

        HBITMAP hbm = futImageProccess.get();
        const char* outText = futTextProccess.get();

        setBitmap( g_hImageControl, hbm );
        resultText( outText );

        frame.release();
    }

    setBitmap( g_hImageControl, g_hPlaceholderBitmap );
    resultText( "" );
}

void openImage() {
    
    OPENFILENAMEA ofn{};

    ZeroMemory( &ofn, sizeof OPENFILENAMEA );

    char szFile[_MAX_PATH] = "";

    ofn.lStructSize = sizeof OPENFILENAMEA;
    ofn.lpstrFile = szFile;
    ofn.lpstrFilter = "Bitmap\0*.bmp*\0JPEG Image\0*.jpg,*.jpeg\0PNG Image\0*.png";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;


    if ( !GetOpenFileNameA(&ofn) ){
        return;
    }

    cv::Mat image;

    image = cv::imread( szFile );

    if ( image.empty() )
        return;

    HBITMAP hbm = convertCvMatToBMP( image );

    if ( !hbm )
        return;

    setBitmap( g_hImageControl, hbm );
}

LRESULT CALLBACK windowProc( HWND hWnd, UINT msg, WPARAM wp, LPARAM lp ) {

    if ( msg == WM_CREATE ) {

    }
    if ( msg == WM_COMMAND ) {

        UINT lwp = LOWORD( wp );
        UINT hwp = HIWORD( wp );

        if ( lwp == IDC_BTN_BROWSE_IMAGEN ) {
            if ( g_isCameraOpen )
                stopCamera();

            openImage();
        }
        if ( lwp == IDC_BTN_CAM ) {

            if(g_isCameraOpen)
                stopCamera();

            initCamera( DEFAULT_CAMERA );
        }
    }
    if (msg == WM_CLOSE) {
        if (g_isCameraOpen)
            stopCamera();
        g_running = false;

        DestroyWindow( g_hMainWindow );
    }

    return DefWindowProc( hWnd, msg, wp, lp );
}

bool initWindow() {
    g_hMainWindow = CreateDialog( g_hInstance, MAKEINTRESOURCE( IDD_MAIN ), 0, windowProc );
    
    if ( g_hMainWindow == NULL )
        return false;

    ShowWindow( g_hMainWindow, SW_SHOW );

    g_hTextControl = GetDlgItem( g_hMainWindow, IDC_TEXT_RESULT );
    g_hImageControl = GetDlgItem( g_hMainWindow, IDC_BMPCONTROL );

    setBitmap(g_hImageControl, g_hPlaceholderBitmap);

#ifdef __DEBUGGING__
    cv::namedWindow(CAPTURE_WINDOW_NAME);
    cv::moveWindow(CAPTURE_WINDOW_NAME, 20, 500);
#endif

    return true;
}

void messageLoop() {

    MSG msg;
    while (g_running) {

        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

}

int main()
{
    char* outText;

    g_tesseractApi = new tesseract::TessBaseAPI();

    // Initialize tesseract-ocr with English, without specifying tessdata path
    if ( g_tesseractApi->Init( NULL, "eng" ) ) {
        fprintf( stderr, "Could not initialize tesseract.\n" );
        exit( 1 );
    }


#ifdef __DEBUGGING__

    // Open input image with leptonica library
    Pix* image = pixRead( "img/dabid.tif" );
    g_tesseractApi->SetImage( image );
    // Get OCR result
    outText = g_tesseractApi->GetUTF8Text();
    printf( "OCR output:\n%s", outText );

#endif

    g_hInstance = GetModuleHandle( NULL );

    g_running = true;

    g_hPlaceholderBitmap = LoadBitmap( g_hInstance, MAKEINTRESOURCE( IDB_PLACEHOLDER ) );

    initWindow();

    messageLoop();
    
    delete g_bitmapEncoder;

    // Destroy used object and release memory
    g_tesseractApi->End();
    delete g_tesseractApi;
    

#ifdef __DEBUGGING__
    delete[] outText;
    pixDestroy( &image );


#endif

    if ( g_thCapture.joinable() )
        g_thCapture.join();

    if ( g_thImageProcessing.joinable() )
        g_thImageProcessing.join();

    return 0;
}