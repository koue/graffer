#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "gd.h"

#include "gdhelpers.h"
#include "png.h"		/* includes zlib.h and setjmp.h */

#define TRUE 1
#define FALSE 0

/*---------------------------------------------------------------------------

    gd_png.c                 Copyright 1999 Greg Roelofs and Thomas Boutell

    The routines in this file, gdImagePng*() and gdImageCreateFromPng*(),
    are drop-in replacements for gdImageGif*() and gdImageCreateFromGif*(),
    except that these functions are noisier in the case of errors (comment
    out all fprintf() statements to disable that).

    GD 2.0 supports RGBA truecolor and will read and write truecolor PNGs.
    GD 2.0 supports 8 bits of color resolution per channel and
    7 bits of alpha channel resolution. Images with more than 8 bits
    per channel are reduced to 8 bits. Images with an alpha channel are
    only able to resolve down to '1/128th opaque' instead of '1/256th',
    and this conversion is also automatic. I very much doubt you can see it.
    Both tRNS and true alpha are supported.

    Gamma is ignored, and there is no support for text annotations.

    Last updated:  9 February 2001

  ---------------------------------------------------------------------------*/

#ifndef PNG_SETJMP_NOT_SUPPORTED
typedef struct _jmpbuf_wrapper
{
  jmp_buf jmpbuf;
}
jmpbuf_wrapper;

static jmpbuf_wrapper gdPngJmpbufStruct;

static void
gdPngErrorHandler (png_structp png_ptr, png_const_charp msg)
{
  jmpbuf_wrapper *jmpbuf_ptr;

  /* This function, aside from the extra step of retrieving the "error
   * pointer" (below) and the fact that it exists within the application
   * rather than within libpng, is essentially identical to libpng's
   * default error handler.  The second point is critical:  since both
   * setjmp() and longjmp() are called from the same code, they are
   * guaranteed to have compatible notions of how big a jmp_buf is,
   * regardless of whether _BSD_SOURCE or anything else has (or has not)
   * been defined. */

  fprintf (stderr, "gd-png:  fatal libpng error: %s\n", msg);
  fflush (stderr);

  jmpbuf_ptr = png_get_error_ptr (png_ptr);
  if (jmpbuf_ptr == NULL) {				/* we are completely hosed now */
      fprintf (stderr, "gd-png:  EXTREMELY fatal error: jmpbuf unrecoverable; terminating.\n");
      fflush (stderr);
      exit (99);
    }

  longjmp (jmpbuf_ptr->jmpbuf, 1);
}
#endif

static void
gdPngWriteData (png_structp png_ptr, png_bytep data, png_size_t length)
{
  gdPutBuf (data, length, (gdIOCtx *) png_get_io_ptr (png_ptr));
}

static void
gdPngFlushData (png_structp png_ptr)
{
}

BGD_DECLARE(void) gdImagePng (gdImagePtr im, FILE * outFile)
{
  gdIOCtx *out = gdNewFileCtx (outFile);
  gdImagePngCtxEx (im, out, -1);
  out->gd_free (out);
}

/* This routine is based in part on code from Dale Lutz (Safe Software Inc.)
 *  and in part on demo code from Chapter 15 of "PNG: The Definitive Guide"
 *  (http://www.cdrom.com/pub/png/pngbook.html).
 */
BGD_DECLARE(void) gdImagePngCtxEx (gdImagePtr im, gdIOCtx * outfile, int level)
{
  int i, j, bit_depth = 0, interlace_type;
  int width = im->sx;
  int height = im->sy;
  int colors = im->colorsTotal;
  int *open = im->open;
  int mapping[gdMaxColors];	/* mapping[gd_index] == png_index */
  png_byte trans_values[256];
  png_color palette[gdMaxColors];
  png_structp png_ptr;
  png_infop info_ptr;
  volatile int remap = FALSE;

#ifndef PNG_SETJMP_NOT_SUPPORTED
  png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING,
				     &gdPngJmpbufStruct, gdPngErrorHandler,
				     NULL);
#else
  png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
#endif
  if (png_ptr == NULL)
    {
      fprintf (stderr, "gd-png error: cannot allocate libpng main struct\n");
      return;
    }

  info_ptr = png_create_info_struct (png_ptr);
  if (info_ptr == NULL)
    {
      fprintf (stderr, "gd-png error: cannot allocate libpng info struct\n");
      png_destroy_write_struct (&png_ptr, (png_infopp) NULL);
      return;
    }

#ifndef PNG_SETJMP_NOT_SUPPORTED
  if (setjmp (gdPngJmpbufStruct.jmpbuf))
    {
      fprintf (stderr, "gd-png error: setjmp returns error condition\n");
      png_destroy_write_struct (&png_ptr, &info_ptr);
      return;
    }
#endif

  png_set_write_fn (png_ptr, (void *) outfile, gdPngWriteData,
		    gdPngFlushData);

  /* This is best for palette images, and libpng defaults to it for
     palette images anyway, so we don't need to do it explicitly.
     What to ideally do for truecolor images depends, alas, on the image.
     gd is intentionally imperfect and doesn't spend a lot of time
     fussing with such things. */

  /* Faster if this is uncommented, but may produce larger truecolor files.
     Wait for gdImagePngCtxEx. */
#if 0
  png_set_filter (png_ptr, 0, PNG_FILTER_NONE);
#endif

  /* 2.0.12: this is finally a parameter */
  png_set_compression_level (png_ptr, level);

  /* can set this to a smaller value without compromising compression if all
   * image data is 16K or less; will save some decoder memory [min == 8] */
/*  png_set_compression_window_bits(png_ptr, 15);  */

  if (!im->trueColor)
    {
      for (i = 0; i < gdMaxColors; ++i)
	mapping[i] = -1;
    }
  if (!im->trueColor)
    {
      /* count actual number of colors used (colorsTotal == high-water mark) */
      colors = 0;
      for (i = 0; i < im->colorsTotal; ++i)
	{
	  if (!open[i])
	    {
	      mapping[i] = colors;
	      ++colors;
	    }
	}
      if (colors < im->colorsTotal)
	{
	  remap = TRUE;
	}
      if (colors <= 2)
	bit_depth = 1;
      else if (colors <= 4)
	bit_depth = 2;
      else if (colors <= 16)
	bit_depth = 4;
      else
	bit_depth = 8;
    }
  interlace_type = im->interlace ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE;

  if (!im->trueColor)
    {
      png_set_IHDR (png_ptr, info_ptr, width, height, bit_depth,
		    PNG_COLOR_TYPE_PALETTE, interlace_type,
		    PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    }
  if (!im->trueColor)
    {
      /* Oy veh. Remap the PNG palette to put the
         entries with interesting alpha channel
         values first. This minimizes the size
         of the tRNS chunk and thus the size
         of the PNG file as a whole. */
      int tc = 0;
      int i;
      int j;
      int k;
      for (i = 0; (i < im->colorsTotal); i++)
	{
	  if ((!im->open[i]) && (im->alpha[i] != gdAlphaOpaque))
	    {
	      tc++;
	    }
	}
      if (tc)
	{
#if 0
	  for (i = 0; (i < im->colorsTotal); i++)
	    {
	      trans_values[i] = 255 -
		((im->alpha[i] << 1) + (im->alpha[i] >> 6));
	    }
	  png_set_tRNS (png_ptr, info_ptr, trans_values, 256, NULL);
#endif
	  if (!remap)
	    {
	      remap = TRUE;
	    }
	  /* (Semi-)transparent indexes come up from the bottom
	     of the list of real colors; opaque
	     indexes come down from the top */
	  j = 0;
	  k = colors - 1;
	  for (i = 0; (i < im->colorsTotal); i++)
	    {
	      if (!im->open[i])
		{
		  if (im->alpha[i] != gdAlphaOpaque)
		    {
		      /* Andrew Hull: >> 6, not >> 7! (gd 2.0.5) */
		      trans_values[j] = 255 -
			((im->alpha[i] << 1) + (im->alpha[i] >> 6));
		      mapping[i] = j++;
		    }
		  else
		    {
		      mapping[i] = k--;
		    }
		}
	    }
	  png_set_tRNS (png_ptr, info_ptr, trans_values, tc, NULL);
	}
    }

  /* convert palette to libpng layout */
  if (!im->trueColor)
    {
      if (remap)
	for (i = 0; i < im->colorsTotal; ++i)
	  {
	    if (mapping[i] < 0)
	      continue;
	    palette[mapping[i]].red = im->red[i];
	    palette[mapping[i]].green = im->green[i];
	    palette[mapping[i]].blue = im->blue[i];
	  }
      else
	for (i = 0; i < colors; ++i)
	  {
	    palette[i].red = im->red[i];
	    palette[i].green = im->green[i];
	    palette[i].blue = im->blue[i];
	  }
      png_set_PLTE (png_ptr, info_ptr, palette, colors);
    }

  /* write out the PNG header info (everything up to first IDAT) */
  png_write_info (png_ptr, info_ptr);

  /* make sure < 8-bit images are packed into pixels as tightly as possible */
  png_set_packing (png_ptr);

  if(!im->trueColor)
    {
      if (remap)
	{
	  png_bytep *row_pointers;
          if (overflow2(sizeof (png_bytep), height)) {
            return;
          }
	  row_pointers = gdMalloc (sizeof (png_bytep) * height);
	  if (row_pointers == NULL)
	    {
	      fprintf (stderr,
		       "gd-png error: unable to allocate row_pointers\n");
              /* TBB: return missing */
              return;
	    }
	  for (j = 0; j < height; ++j)
	    {
	      if ((row_pointers[j] = (png_bytep) gdMalloc (width)) == NULL)
		{
		  fprintf (stderr, "gd-png error: unable to allocate rows\n");
		  for (i = 0; i < j; ++i)
		    gdFree (row_pointers[i]);
                  /* TBB: memory leak */
		  gdFree (row_pointers);
		  return;
		}
	      for (i = 0; i < width; ++i)
		row_pointers[j][i] = mapping[im->pixels[j][i]];
	    }

	  png_write_image (png_ptr, row_pointers);
	  png_write_end (png_ptr, info_ptr);

	  for (j = 0; j < height; ++j)
	    gdFree (row_pointers[j]);
	  gdFree (row_pointers);
	}
      else
	{
	  png_write_image (png_ptr, im->pixels);
	  png_write_end (png_ptr, info_ptr);
	}
    }
  /* 1.6.3: maybe we should give that memory BACK! TBB */
  png_destroy_write_struct (&png_ptr, &info_ptr);
}
