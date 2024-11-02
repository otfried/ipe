// ipebitmap_win.cpp
/*

	This file is part of the extensible drawing editor Ipe.
	Copyright (c) 1993-2024 Otfried Cheong

	Ipe is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	As a special exception, you have permission to link Ipe with the
	CGAL library and distribute executables, as long as you follow the
	requirements of the Gnu General Public License in regard to all of
	the software in the executable aside from CGAL.

	Ipe is distributed in the hope that it will be useful, but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
	or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
	License for more details.

	You should have received a copy of the GNU General Public License
	along with Ipe; if not, you can find it at
	"http://www.gnu.org/copyleft/gpl.html", or write to the Free
	Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "ipebitmap.h"
#include "ipeutils.h"

#include <windows.h>
#include <windowsx.h>
// must be before this
#include <gdiplus.h>

using namespace ipe;

// --------------------------------------------------------------------

typedef IStream * WINAPI (*LPFNSHCREATEMEMSTREAM)(const BYTE *, UINT);
static bool libLoaded = false;
static LPFNSHCREATEMEMSTREAM pSHCreateMemStream = nullptr;

bool dctDecode(Buffer dctData, Buffer pixelData) {
    if (!libLoaded) {
	libLoaded = true;
	HMODULE hDll = LoadLibraryA("shlwapi.dll");
	if (hDll)
	    pSHCreateMemStream = (LPFNSHCREATEMEMSTREAM)GetProcAddress(hDll, (LPCSTR)12);
    }
    if (!pSHCreateMemStream) return false;

    IStream * stream = pSHCreateMemStream((const BYTE *)dctData.data(), dctData.size());
    if (!stream) return false;

    Gdiplus::Bitmap * bitmap = Gdiplus::Bitmap::FromStream(stream);
    if (bitmap->GetLastStatus() != Gdiplus::Ok) {
	delete bitmap;
	stream->Release();
	return false;
    }

    int w = bitmap->GetWidth();
    int h = bitmap->GetHeight();
    // ipeDebug("dctDecode: %d x %d format %x", w, h, bitmap->GetPixelFormat());

    Gdiplus::BitmapData * bitmapData = new Gdiplus::BitmapData;
    bitmapData->Scan0 = pixelData.data();
    bitmapData->Stride = 4 * w;
    bitmapData->Width = w;
    bitmapData->Height = h;
    bitmapData->PixelFormat = PixelFormat32bppARGB;

    Gdiplus::Rect rect(0, 0, w, h);
    bitmap->LockBits(&rect,
		     Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeUserInputBuf,
		     PixelFormat32bppARGB, bitmapData);
    bitmap->UnlockBits(bitmapData);
    delete bitmapData;
    delete bitmap;
    stream->Release();

    return true;
}

// --------------------------------------------------------------------
// The graphics file formats supported by GDI+ are
// BMP, GIF, JPEG, PNG, TIFF.
Bitmap Bitmap::readPNG(const char * fname, Vector & dotsPerInch, const char *& errmsg) {
    // load without color correction
    Gdiplus::Bitmap * bitmap = Gdiplus::Bitmap::FromFile(String(fname).w().data(), FALSE);
    if (bitmap->GetLastStatus() != Gdiplus::Ok) {
	delete bitmap;
	return Bitmap();
    }

    dotsPerInch =
	Vector(bitmap->GetHorizontalResolution(), bitmap->GetVerticalResolution());

    int w = bitmap->GetWidth();
    int h = bitmap->GetHeight();

    Buffer pixelData(4 * w * h);
    Gdiplus::BitmapData * bitmapData = new Gdiplus::BitmapData;
    bitmapData->Scan0 = pixelData.data();
    bitmapData->Stride = 4 * w;
    bitmapData->Width = w;
    bitmapData->Height = h;
    bitmapData->PixelFormat = PixelFormat32bppARGB;

    Gdiplus::Rect rect(0, 0, w, h);
    bitmap->LockBits(&rect,
		     Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeUserInputBuf,
		     PixelFormat32bppARGB, bitmapData);

    Bitmap bm(w, h, Bitmap::ENative, pixelData);

    bitmap->UnlockBits(bitmapData);
    delete bitmapData;
    delete bitmap;

    return bm;
}

// --------------------------------------------------------------------
