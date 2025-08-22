#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
/* 2.03: don't include zlib here or we can't build without PNG */
#include "gd.h"
#include "gdhelpers.h"

/* 2.0.12: this now checks the clipping rectangle */
#define gdImageBoundsSafeMacro(im, x, y) (!((((y) < (im)->cy1) || ((y) > (im)->cy2)) || (((x) < (im)->cx1) || ((x) > (im)->cx2))))

#ifdef _OSD_POSIX		/* BS2000 uses the EBCDIC char set instead of ASCII */
#define CHARSET_EBCDIC
#define __attribute__(any)	/*nothing */
#endif
/*_OSD_POSIX*/

#ifndef CHARSET_EBCDIC
#define ASC(ch)  ch
#else /*CHARSET_EBCDIC */
#define ASC(ch) gd_toascii[(unsigned char)ch]
static const unsigned char gd_toascii[256] = {
/*00 */ 0x00, 0x01, 0x02, 0x03, 0x85, 0x09, 0x86, 0x7f,
  0x87, 0x8d, 0x8e, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,	/*................ */
/*10 */ 0x10, 0x11, 0x12, 0x13, 0x8f, 0x0a, 0x08, 0x97,
  0x18, 0x19, 0x9c, 0x9d, 0x1c, 0x1d, 0x1e, 0x1f,	/*................ */
/*20 */ 0x80, 0x81, 0x82, 0x83, 0x84, 0x92, 0x17, 0x1b,
  0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x05, 0x06, 0x07,	/*................ */
/*30 */ 0x90, 0x91, 0x16, 0x93, 0x94, 0x95, 0x96, 0x04,
  0x98, 0x99, 0x9a, 0x9b, 0x14, 0x15, 0x9e, 0x1a,	/*................ */
/*40 */ 0x20, 0xa0, 0xe2, 0xe4, 0xe0, 0xe1, 0xe3, 0xe5,
  0xe7, 0xf1, 0x60, 0x2e, 0x3c, 0x28, 0x2b, 0x7c,	/* .........`.<(+| */
/*50 */ 0x26, 0xe9, 0xea, 0xeb, 0xe8, 0xed, 0xee, 0xef,
  0xec, 0xdf, 0x21, 0x24, 0x2a, 0x29, 0x3b, 0x9f,	/*&.........!$*);. */
/*60 */ 0x2d, 0x2f, 0xc2, 0xc4, 0xc0, 0xc1, 0xc3, 0xc5,
  0xc7, 0xd1, 0x5e, 0x2c, 0x25, 0x5f, 0x3e, 0x3f,
/*-/........^,%_>?*/
/*70 */ 0xf8, 0xc9, 0xca, 0xcb, 0xc8, 0xcd, 0xce, 0xcf,
  0xcc, 0xa8, 0x3a, 0x23, 0x40, 0x27, 0x3d, 0x22,	/*..........:#@'=" */
/*80 */ 0xd8, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
  0x68, 0x69, 0xab, 0xbb, 0xf0, 0xfd, 0xfe, 0xb1,	/*.abcdefghi...... */
/*90 */ 0xb0, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
  0x71, 0x72, 0xaa, 0xba, 0xe6, 0xb8, 0xc6, 0xa4,	/*.jklmnopqr...... */
/*a0 */ 0xb5, 0xaf, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
  0x79, 0x7a, 0xa1, 0xbf, 0xd0, 0xdd, 0xde, 0xae,	/*..stuvwxyz...... */
/*b0 */ 0xa2, 0xa3, 0xa5, 0xb7, 0xa9, 0xa7, 0xb6, 0xbc,
  0xbd, 0xbe, 0xac, 0x5b, 0x5c, 0x5d, 0xb4, 0xd7,	/*...........[\].. */
/*c0 */ 0xf9, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
  0x48, 0x49, 0xad, 0xf4, 0xf6, 0xf2, 0xf3, 0xf5,	/*.ABCDEFGHI...... */
/*d0 */ 0xa6, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
  0x51, 0x52, 0xb9, 0xfb, 0xfc, 0xdb, 0xfa, 0xff,	/*.JKLMNOPQR...... */
/*e0 */ 0xd9, 0xf7, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
  0x59, 0x5a, 0xb2, 0xd4, 0xd6, 0xd2, 0xd3, 0xd5,	/*..STUVWXYZ...... */
/*f0 */ 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
  0x38, 0x39, 0xb3, 0x7b, 0xdc, 0x7d, 0xda, 0x7e	/*0123456789.{.}.~ */
};
#endif /*CHARSET_EBCDIC */

BGD_DECLARE(gdImagePtr) gdImageCreate (int sx, int sy)
{
  int i;
  gdImagePtr im;

  if (overflow2(sizeof (unsigned char *), sy)) {
		return NULL;
  }
  if (overflow2(sizeof (unsigned char *), sx)) {
		return NULL;
  }

  im = (gdImage *) gdMalloc (sizeof (gdImage));
	if (!im) {
		return NULL;
	}

  memset (im, 0, sizeof (gdImage));
  /* Row-major ever since gd 1.3 */
  im->pixels = (unsigned char **) gdMalloc (sizeof (unsigned char *) * sy);
	if (!im->pixels) {
		gdFree(im);
		return NULL;
	}

  for (i = 0; (i < sy); i++)
    {
      /* Row-major ever since gd 1.3 */
      im->pixels[i] = (unsigned char *) gdCalloc (sx, sizeof (unsigned char));
			if (!im->pixels[i])
			{
				for (--i ; i >= 0; i--)
				{
					gdFree(im->pixels[i]);
				}
				gdFree(im->pixels);
				gdFree(im);
				return NULL;
			}

    }
  im->sx = sx;
  im->sy = sy;
  im->colorsTotal = 0;
  im->thick = 1;
  for (i = 0; (i < gdMaxColors); i++)
    {
      im->open[i] = 1;
      im->red[i] = 0;
      im->green[i] = 0;
      im->blue[i] = 0;
    };
  im->cx1 = 0;
  im->cy1 = 0;
  im->cx2 = im->sx - 1;
  im->cy2 = im->sy - 1;
  return im;
}

BGD_DECLARE(void) gdImageDestroy (gdImagePtr im)
{
  int i;
  if (im->pixels)
    {
      for (i = 0; (i < im->sy); i++)
	{
	  gdFree (im->pixels[i]);
	}
      gdFree (im->pixels);
    }
  gdFree (im);
}

BGD_DECLARE(int) gdImageColorAllocate (gdImagePtr im, int r, int g, int b)
{
  return gdImageColorAllocateAlpha (im, r, g, b, gdAlphaOpaque);
}

BGD_DECLARE(int) gdImageColorAllocateAlpha (gdImagePtr im, int r, int g, int b, int a)
{
  int i;
  int ct = (-1);
  for (i = 0; (i < (im->colorsTotal)); i++)
    {
      if (im->open[i])
	{
	  ct = i;
	  break;
	}
    }
  if (ct == (-1))
    {
      ct = im->colorsTotal;
      if (ct == gdMaxColors)
	{
	  return -1;
	}
      im->colorsTotal++;
    }
  im->red[ct] = r;
  im->green[ct] = g;
  im->blue[ct] = b;
  im->open[ct] = 0;
  return ct;
}

BGD_DECLARE(void) gdImageColorDeallocate (gdImagePtr im, int color)
{
  /* Mark it open. */
  im->open[color] = 1;
}

static int
clip_1d (int *x0, int *y0, int *x1, int *y1, int mindim, int maxdim)
{
  double m;			/* gradient of line */
  if (*x0 < mindim)
    {				/* start of line is left of window */
      if (*x1 < mindim)		/* as is the end, so the line never cuts the window */
	return 0;
      m = (*y1 - *y0) / (double) (*x1 - *x0);	/* calculate the slope of the line */
      /* adjust x0 to be on the left boundary (ie to be zero), and y0 to match */
      *y0 -= m * (*x0 - mindim);
      *x0 = mindim;
      /* now, perhaps, adjust the far end of the line as well */
      if (*x1 > maxdim)
	{
	  *y1 += m * (maxdim - *x1);
	  *x1 = maxdim;
	}
      return 1;
    }
  if (*x0 > maxdim)
    {				/* start of line is right of window -
				   complement of above */
      if (*x1 > maxdim)		/* as is the end, so the line misses the window */
	return 0;
      m = (*y1 - *y0) / (double) (*x1 - *x0);	/* calculate the slope of the line */
      *y0 += m * (maxdim - *x0);	/* adjust so point is on the right
					   boundary */
      *x0 = maxdim;
      /* now, perhaps, adjust the end of the line */
      if (*x1 < mindim)
	{
	  *y1 -= m * (*x1 - mindim);
	  *x1 = mindim;
	}
      return 1;
    }
  /* the final case - the start of the line is inside the window */
  if (*x1 > maxdim)
    {				/* other end is outside to the right */
      m = (*y1 - *y0) / (double) (*x1 - *x0);	/* calculate the slope of the line */
      *y1 += m * (maxdim - *x1);
      *x1 = maxdim;
      return 1;
    }
  if (*x1 < mindim)
    {				/* other end is outside to the left */
      m = (*y1 - *y0) / (double) (*x1 - *x0);	/* calculate the slope of the line */
      *y1 -= m * (*x1 - mindim);
      *x1 = mindim;
      return 1;
    }
  /* only get here if both points are inside the window */
  return 1;
}

/* end of line clipping code */

BGD_DECLARE(void) gdImageSetPixel (gdImagePtr im, int x, int y, int color)
{
  switch (color)
    {
    default:
      if (gdImageBoundsSafeMacro (im, x, y))
	{
	  im->pixels[y][x] = color;
	}
      break;
    }
}

static void gdImageHLine(gdImagePtr im, int y, int x1, int x2, int col)
{
	if (im->thick > 1) {
		int thickhalf = im->thick >> 1;
		gdImageFilledRectangle(im, x1, y - thickhalf, x2, y + im->thick - thickhalf - 1, col);
	} else {
		if (x2 < x1) {
			int t = x2;
			x2 = x1;
			x1 = t;
		}

		for (;x1 <= x2; x1++) {
			gdImageSetPixel(im, x1, y, col);
		}
	}
	return;
}

static void gdImageVLine(gdImagePtr im, int x, int y1, int y2, int col)
{
	if (im->thick > 1) {
		int thickhalf = im->thick >> 1;
		gdImageFilledRectangle(im, x - thickhalf, y1, x + im->thick - thickhalf - 1, y2, col);
	} else {
		if (y2 < y1) {
			int t = y1;
			y1 = y2;
			y2 = t;
		}

		for (;y1 <= y2; y1++) {
			gdImageSetPixel(im, x, y1, col);
		}
	}
	return;
}

/* Bresenham as presented in Foley & Van Dam */
BGD_DECLARE(void) gdImageLine (gdImagePtr im, int x1, int y1, int x2, int y2, int color)
{
  int dx, dy, incr1, incr2, d, x, y, xend, yend, xdirflag, ydirflag;
  int wid;
  int w, wstart;
  int thick;

  /* 2.0.10: Nick Atty: clip to edges of drawing rectangle, return if no
     points need to be drawn. 2.0.26, TBB: clip to edges of clipping
     rectangle. We were getting away with this because gdImageSetPixel
     is used for actual drawing, but this is still more efficient and opens
     the way to skip per-pixel bounds checking in the future. */

  if (clip_1d (&x1, &y1, &x2, &y2, im->cx1, im->cx2) == 0)
    return;
  if (clip_1d (&y1, &x1, &y2, &x2, im->cy1, im->cy2) == 0)
    return;
  thick = im->thick;

  dx = abs (x2 - x1);
  dy = abs (y2 - y1);

	if (dx == 0) {
		gdImageVLine(im, x1, y1, y2, color);
		return;
	} else if (dy == 0) {
		gdImageHLine(im, y1, x1, x2, color);
		return;
	}

  if (dy <= dx)
    {
      /* More-or-less horizontal. use wid for vertical stroke */
      /* Doug Claar: watch out for NaN in atan2 (2.0.5) */
      if ((dx == 0) && (dy == 0))
	{
	  wid = 1;
	}
      else
	{
	  /* 2.0.12: Michael Schwartz: divide rather than multiply;
	     TBB: but watch out for /0! */
	  double ac = cos (atan2 (dy, dx));
	  if (ac != 0)
	    {
	      wid = thick / ac;
	    }
	  else
	    {
	      wid = 1;
	    }
	  if (wid == 0)
	    {
	      wid = 1;
	    }
	}
      d = 2 * dy - dx;
      incr1 = 2 * dy;
      incr2 = 2 * (dy - dx);
      if (x1 > x2)
	{
	  x = x2;
	  y = y2;
	  ydirflag = (-1);
	  xend = x1;
	}
      else
	{
	  x = x1;
	  y = y1;
	  ydirflag = 1;
	  xend = x2;
	}

      /* Set up line thickness */
      wstart = y - wid / 2;
      for (w = wstart; w < wstart + wid; w++)
	gdImageSetPixel (im, x, w, color);

      if (((y2 - y1) * ydirflag) > 0)
	{
	  while (x < xend)
	    {
	      x++;
	      if (d < 0)
		{
		  d += incr1;
		}
	      else
		{
		  y++;
		  d += incr2;
		}
	      wstart = y - wid / 2;
	      for (w = wstart; w < wstart + wid; w++)
		gdImageSetPixel (im, x, w, color);
	    }
	}
      else
	{
	  while (x < xend)
	    {
	      x++;
	      if (d < 0)
		{
		  d += incr1;
		}
	      else
		{
		  y--;
		  d += incr2;
		}
	      wstart = y - wid / 2;
	      for (w = wstart; w < wstart + wid; w++)
		gdImageSetPixel (im, x, w, color);
	    }
	}
    }
  else
    {
      /* More-or-less vertical. use wid for horizontal stroke */
      /* 2.0.12: Michael Schwartz: divide rather than multiply;
         TBB: but watch out for /0! */
      double as = sin (atan2 (dy, dx));
      if (as != 0)
	{
	  wid = thick / as;
	}
      else
	{
	  wid = 1;
	}
      if (wid == 0)
	wid = 1;

      d = 2 * dx - dy;
      incr1 = 2 * dx;
      incr2 = 2 * (dx - dy);
      if (y1 > y2)
	{
	  y = y2;
	  x = x2;
	  yend = y1;
	  xdirflag = (-1);
	}
      else
	{
	  y = y1;
	  x = x1;
	  yend = y2;
	  xdirflag = 1;
	}

      /* Set up line thickness */
      wstart = x - wid / 2;
      for (w = wstart; w < wstart + wid; w++)
	gdImageSetPixel (im, w, y, color);

      if (((x2 - x1) * xdirflag) > 0)
	{
	  while (y < yend)
	    {
	      y++;
	      if (d < 0)
		{
		  d += incr1;
		}
	      else
		{
		  x++;
		  d += incr2;
		}
	      wstart = x - wid / 2;
	      for (w = wstart; w < wstart + wid; w++)
		gdImageSetPixel (im, w, y, color);
	    }
	}
      else
	{
	  while (y < yend)
	    {
	      y++;
	      if (d < 0)
		{
		  d += incr1;
		}
	      else
		{
		  x--;
		  d += incr2;
		}
	      wstart = x - wid / 2;
	      for (w = wstart; w < wstart + wid; w++)
		gdImageSetPixel (im, w, y, color);
	    }
	}
    }

}

BGD_DECLARE(int) gdImageBoundsSafe (gdImagePtr im, int x, int y)
{
  return gdImageBoundsSafeMacro (im, x, y);
}

BGD_DECLARE(void) gdImageChar (gdImagePtr im, gdFontPtr f, int x, int y, int c, int color)
{
  int cx, cy;
  int px, py;
  int fline;
  cx = 0;
  cy = 0;
#ifdef CHARSET_EBCDIC
  c = ASC (c);
#endif /*CHARSET_EBCDIC */
  if ((c < f->offset) || (c >= (f->offset + f->nchars)))
    {
      return;
    }
  fline = (c - f->offset) * f->h * f->w;
  for (py = y; (py < (y + f->h)); py++)
    {
      for (px = x; (px < (x + f->w)); px++)
	{
	  if (f->data[fline + cy * f->w + cx])
	    {
	      gdImageSetPixel (im, px, py, color);
	    }
	  cx++;
	}
      cx = 0;
      cy++;
    }
}

BGD_DECLARE(void) gdImageCharUp (gdImagePtr im, gdFontPtr f, int x, int y, int c, int color)
{
  int cx, cy;
  int px, py;
  int fline;
  cx = 0;
  cy = 0;
#ifdef CHARSET_EBCDIC
  c = ASC (c);
#endif /*CHARSET_EBCDIC */
  if ((c < f->offset) || (c >= (f->offset + f->nchars)))
    {
      return;
    }
  fline = (c - f->offset) * f->h * f->w;
  for (py = y; (py > (y - f->w)); py--)
    {
      for (px = x; (px < (x + f->h)); px++)
	{
	  if (f->data[fline + cy * f->w + cx])
	    {
	      gdImageSetPixel (im, px, py, color);
	    }
	  cy++;
	}
      cy = 0;
      cx++;
    }
}

BGD_DECLARE(void) gdImageString (gdImagePtr im, gdFontPtr f,
	       int x, int y, unsigned char *s, int color)
{
  int i;
  int l;
  l = strlen ((char *) s);
  for (i = 0; (i < l); i++)
    {
      gdImageChar (im, f, x, y, s[i], color);
      x += f->w;
    }
}

BGD_DECLARE(void) gdImageStringUp (gdImagePtr im, gdFontPtr f,
		 int x, int y, unsigned char *s, int color)
{
  int i;
  int l;
  l = strlen ((char *) s);
  for (i = 0; (i < l); i++)
    {
      gdImageCharUp (im, f, x, y, s[i], color);
      y -= f->w;
    }
}

BGD_DECLARE(void) gdImageFilledRectangle (gdImagePtr im, int x1, int y1, int x2, int y2,
			int color)
{
  int x, y;
  /* Nick Atty: limit the points at the edge.  Note that this also
     nicely kills any plotting for rectangles completely outside the
     window as it makes the tests in the for loops fail */
  if (x1 < 0)
	  x1 = 0;
  if (x1 > gdImageSX (im))
	  x1 = gdImageSX (im);
  if (y1 < 0)
	  y1 = 0;
  if (y1 > gdImageSY (im))
	  y1 = gdImageSY (im);

  if (x1 > x2) {
	  x = x1;
	  x1 = x2;
	  x2 = x;
  }
  if (y1 > y2) {
	  y = y1;
	  y1 = y2;
	  y2 = y;
  }

  for (y = y1; (y <= y2); y++)
    {
      for (x = x1; (x <= x2); x++)
	{
	  gdImageSetPixel (im, x, y, color);
	}
    }
}
