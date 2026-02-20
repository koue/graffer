#ifndef GD_H
#define GD_H 1

#define GD_MAJOR_VERSION 2
#define GD_MINOR_VERSION 0
#define GD_RELEASE_VERSION 35
#define GD_EXTRA_VERSION ""
#define GD_VERSION_STRING "2.0.35"

#define BGD_DECLARE(rt) extern rt
#define BGD_EXPORT_DATA_PROT extern
#define BGD_EXPORT_DATA_IMPL

/* stdio is needed for file I/O. */
#include <stdio.h>
#include "gd_io.h"

#define gdMaxColors 256

  typedef struct gdImageStruct
  {
    /* Palette-based image pixels */
    unsigned char **pixels;
    int sx;
    int sy;
    int colorsTotal;
    int red[gdMaxColors];
    int green[gdMaxColors];
    int blue[gdMaxColors];
    int open[gdMaxColors];
    int thick;

    int cx1;
    int cy1;
    int cx2;
    int cy2;
  }
  gdImage;

  typedef gdImage *gdImagePtr;

  typedef struct
  {
    /* # of characters in font */
    int nchars;
    /* First character is numbered... (usually 32 = space) */
    int offset;
    /* Character width and height */
    int w;
    int h;
    /* Font data; array of characters, one row after another.
       Easily included in code, also easily loaded from
       data files. */
    char *data;
  }
  gdFont;

/* Text functions take these. */
  typedef gdFont *gdFontPtr;

/* Functions to manipulate images. */

/* Creates a palette-based image (up to 256 colors). */
BGD_DECLARE(gdImagePtr) gdImageCreate (int sx, int sy);

BGD_DECLARE(void) gdImageDestroy (gdImagePtr im);

BGD_DECLARE(void) gdImageSetPixel (gdImagePtr im, int x, int y, int color);

BGD_DECLARE(void) gdImageLine (gdImagePtr im, int x1, int y1, int x2, int y2, int color);

/* Solid bar. Upper left corner first, lower right corner second. */
BGD_DECLARE(void) gdImageFilledRectangle (gdImagePtr im, int x1, int y1, int x2, int y2,
			       int color);
BGD_DECLARE(int) gdImageBoundsSafe (gdImagePtr im, int x, int y);
BGD_DECLARE(void) gdImageChar (gdImagePtr im, gdFontPtr f, int x, int y, int c,
		    int color);
BGD_DECLARE(void) gdImageCharUp (gdImagePtr im, gdFontPtr f, int x, int y, int c,
		      int color);
BGD_DECLARE(void) gdImageString (gdImagePtr im, gdFontPtr f, int x, int y,
		      unsigned char *s, int color);
BGD_DECLARE(void) gdImageStringUp (gdImagePtr im, gdFontPtr f, int x, int y,
			unsigned char *s, int color);

BGD_DECLARE(int) gdImageColorAllocate (gdImagePtr im, int r, int g, int b);

BGD_DECLARE(void) gdImageColorDeallocate (gdImagePtr im, int color);

BGD_DECLARE(void) gdImagePng (gdImagePtr im, FILE * out);
BGD_DECLARE(void) gdImagePngCtx (gdImagePtr im, gdIOCtx * out);
BGD_DECLARE(void) gdImagePngCtxEx (gdImagePtr im, gdIOCtx * out, int level);

/* Guaranteed to correctly free memory returned
	by the gdImage*Ptr functions */
BGD_DECLARE(void) gdFree (void *m);

#define gdImageSX(im) ((im)->sx)
#define gdImageSY(im) ((im)->sy)

/* I/O Support routines. */

BGD_DECLARE(gdIOCtx *) gdNewFileCtx (FILE *);
#endif				/* GD_H */
