// ipebitmap_unix.cpp
// Code dependent on libjpeg (Linux only) and libpng
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

#include <png.h>

#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#else
#include <csetjmp>
#include <jpeglib.h>
#endif

using namespace ipe;

// --------------------------------------------------------------------

#ifdef __APPLE__

bool cgImageDecode(CGImageRef bitmap, Buffer pixelData)
{
  if (CGImageGetBitsPerComponent(bitmap) != 8)
    return false;

  int w = CGImageGetWidth(bitmap);
  int h = CGImageGetHeight(bitmap);
  int bits = CGImageGetBitsPerPixel(bitmap);
  int stride = CGImageGetBytesPerRow(bitmap);
  if (bits != 8 && bits != 24 && bits != 32)
    return false;

  CGBitmapInfo info = CGImageGetBitmapInfo(bitmap);
  // Do we need to check for float pixel values and byte order?
  ipeDebug("cgImageDecode: %d x %d x %d, stride %d, info %x", w, h, bits, stride, info);

  CFDataRef pixels = CGDataProviderCopyData(CGImageGetDataProvider(bitmap));
  const uint8_t *inRow = CFDataGetBytePtr(pixels);

  uint32_t *q = (uint32_t *) pixelData.data();
  if (bits == 24) {
    for (int y = 0; y < h; ++y) {
      const uint8_t *p = inRow;
      for (int x = 0; x < w; ++x) {
	*q++ = 0xff000000 | (p[0] << 16) | (p[1] << 8) | p[2];
	p += 3;
      }
      inRow += stride;
    }
  } else if (bits ==  32) {
    for (int y = 0; y < h; ++y) {
      const uint8_t *p = inRow;
      for (int x = 0; x < w; ++x) {
  	*q++ = (p[3]<<24) | (p[0] << 16) | (p[1] << 8) | p[2];
  	p += 4;
      }
      inRow += stride;
    }
  } else {  // bits == 8
    for (int y = 0; y < h; ++y) {
      const uint8_t *p = inRow;
      for (int x = 0; x < w; ++x) {
	*q++ = 0xff000000 | (*p << 16) | (*p << 8) | *p;
	++p;
      }
      inRow += stride;
    }
  }
  CFRelease(pixels);
  return true;
}

bool dctDecode(Buffer dctData, Buffer pixelData)
{
  CGDataProviderRef source =
    CGDataProviderCreateWithData(nullptr, dctData.data(), dctData.size(), nullptr);
  CGImageRef bitmap =
    CGImageCreateWithJPEGDataProvider(source, nullptr, false, kCGRenderingIntentDefault);
  int result = cgImageDecode(bitmap, pixelData);
  CGImageRelease(bitmap);
  CGDataProviderRelease(source);
  return result;
}

#else

// Decode jpeg image using libjpeg API with error handling
// Code contributed by Michael Thon, 2015.

// The following is error-handling code for decompressing jpeg using the
// standard libjpeg API. Taken from the example.c and stackoverflow.
struct jpegErrorManager {
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

static char jpegLastErrorMsg[JMSG_LENGTH_MAX];

static void jpegErrorExit (j_common_ptr cinfo)
{
  jpegErrorManager *myerr = (jpegErrorManager*) cinfo->err;
  (*(cinfo->err->format_message)) (cinfo, jpegLastErrorMsg);
  longjmp(myerr->setjmp_buffer, 1);
}

bool dctDecode(Buffer dctData, Buffer pixelData)
{
  struct jpeg_decompress_struct cinfo;

  // Error handling:
  struct jpegErrorManager jerr;
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = jpegErrorExit;
  if (setjmp(jerr.setjmp_buffer)) {
    ipeDebug("jpeg decompression failed: %s", jpegLastErrorMsg);
    jpeg_destroy_decompress(&cinfo);
    return false;
  }
  // Decompression:
  jpeg_create_decompress(&cinfo);
  jpeg_mem_src(&cinfo, (unsigned char *) dctData.data(), dctData.size());
  jpeg_read_header(&cinfo, 1);
  cinfo.out_color_space = JCS_RGB;
  jpeg_start_decompress(&cinfo);
  uint32_t *p = (uint32_t *) pixelData.data();
  Buffer row(cinfo.output_width * cinfo.output_components);
  uint8_t *buffer[1];
  uint8_t *fin = (uint8_t *) row.data() + row.size();
  while (cinfo.output_scanline < cinfo.output_height) {
    buffer[0] = (uint8_t *) row.data();
    jpeg_read_scanlines(&cinfo, buffer, 1);
    uint32_t pixel;
    uint8_t *q = (uint8_t *) row.data();
    while (q < fin) {
      pixel = 0xff000000 | (*q++ << 16);
      pixel |= (*q++ << 8);
      *p++ = pixel | *q++;
    }
  }
  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  return true;
}
#endif

// --------------------------------------------------------------------

//! Read PNG image from file.
/*! Returns the image as a Bitmap.
  It will be compressed if \a deflate is set.
  Sets \a dotsPerInch if the image file contains a resolution,
  otherwise sets it to (0,0).
  If reading the file fails, returns a null Bitmap,
  and sets the error message \a errmsg.
*/
Bitmap Bitmap::readPNG(const char *fname, Vector &dotsPerInch, const char * &errmsg)
{
  FILE *fp = Platform::fopen(fname, "rb");
  if (!fp) {
    errmsg = "Error opening file";
    return Bitmap();
  }

  static const char pngerr[] = "PNG library error";
  uint8_t header[8];
  if (fread(header, 1, 8, fp) != 8 ||
      png_sig_cmp(header, 0, 8)) {
    errmsg = "The file does not appear to be a PNG image";
    fclose(fp);
    return Bitmap();
  }

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
					       (png_voidp) nullptr, nullptr, nullptr);
  if (!png_ptr) {
    errmsg = pngerr;
    fclose(fp);
    return Bitmap();
  }
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, (png_infopp) nullptr, (png_infopp) nullptr);
    errmsg = pngerr;
    return Bitmap();
  }
  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) nullptr);
    errmsg = pngerr;
    fclose(fp);
    return Bitmap();
  }

#if PNG_LIBPNG_VER >= 10504
  png_set_alpha_mode(png_ptr, PNG_ALPHA_PNG, PNG_GAMMA_LINEAR);
#endif

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);
  png_read_info(png_ptr, info_ptr);

  int width = png_get_image_width(png_ptr, info_ptr);
  int height = png_get_image_height(png_ptr, info_ptr);
  int color_type = png_get_color_type(png_ptr, info_ptr);

  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
      png_set_tRNS_to_alpha(png_ptr);
      png_set_swap_alpha(png_ptr);
    } else
      png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_BEFORE);
  } else {
    if (color_type == PNG_COLOR_TYPE_GRAY && png_get_bit_depth(png_ptr, info_ptr) < 8)
      png_set_expand_gray_1_2_4_to_8(png_ptr);

    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      png_set_gray_to_rgb(png_ptr);

    if (color_type & PNG_COLOR_MASK_ALPHA)
      png_set_swap_alpha(png_ptr);
    else
      png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_BEFORE);
  }

  if (png_get_bit_depth(png_ptr, info_ptr) == 16)
#if PNG_LIBPNG_VER >= 10504
    png_set_scale_16(png_ptr);
#else
    png_set_strip_16(png_ptr);
#endif

  png_read_update_info(png_ptr, info_ptr);
  if (png_get_bit_depth(png_ptr, info_ptr) != 8) {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) nullptr);
    errmsg = "Depth of PNG image is not eight bits.";
    fclose(fp);
    return Bitmap();
  }

  const double mpi = 25.4/1000.0;
  dotsPerInch = Vector(mpi * png_get_x_pixels_per_meter(png_ptr, info_ptr),
		       mpi * png_get_y_pixels_per_meter(png_ptr, info_ptr));

  Buffer pixels(4 * width * height);
  png_bytep row[height];
  for (int y = 0; y < height; ++y)
    row[y] = (png_bytep) pixels.data() + 4 * width * y;
  png_read_image(png_ptr, row);

  png_read_end(png_ptr, (png_infop) nullptr);
  png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) nullptr);
  fclose(fp);

  Bitmap bm(width, height, Bitmap::ERGB|Bitmap::EAlpha, pixels);
  return bm;
}

// --------------------------------------------------------------------
