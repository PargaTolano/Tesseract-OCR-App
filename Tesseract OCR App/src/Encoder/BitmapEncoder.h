#pragma once

#ifndef __BITMAP_ENCODER_CLASS_

#define __BITMAP_ENCODER_CLASS_

#include <leptonica/allheaders.h>
#include <Windows.h>
#include <gdiplus.h>

#define GET_BIT( x, y ) (unsigned char)((x) >> (y) )
#define FIRST_BIT  ( x )   GET_BIT( (x) , 24 )
#define SECOND_BIT ( x )  GET_BIT( (x) , 16 )
#define THIRD_BIT  ( x )   GET_BIT( (x) , 8 )
#define FOURTH_BIT ( x )  GET_BIT( (x) , 0 )


namespace Parga_PI {
	

	class BitmapEncoder {
	private:

		Gdiplus::Bitmap* m_bitmap;

	public:

		Gdiplus::Bitmap* pixToGdiplusBitmap( Pix* image, Gdiplus::PixelFormat pixelFormat ) {

			l_int32 w = pixGetWidth(image),
				h = pixGetHeight(image),
				d = pixGetDepth(image),
				stride = d * w;

			BYTE* data0 = (BYTE*)pixGetData(image);

#ifdef __DEBUGGING__

			printf( "%%%%%%%%%\ndepth: %d; r: %d; g: %d; b: %d;\n%%%%%%%%%", d, data0[0], data0[1], data0[2]);

#endif

			return new Gdiplus::Bitmap(w, h, stride, pixelFormat, data0);
		}


	};

}


#endif // __BITMAP_ENCODER_CLASS_