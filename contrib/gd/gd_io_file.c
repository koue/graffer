/*
   * io_file.c
   *
   * Implements the file interface.
   *
   * As will all I/O modules, most functions are for local use only (called
   * via function pointers in the I/O context).
   *
   * Most functions are just 'wrappers' for standard file functions.
   *
   * Written/Modified 1999, Philip Warner.
   *
 */

#include "gd.h"
#include "gdhelpers.h"

/* this is used for creating images in main memory */

typedef struct fileIOCtx
{
  gdIOCtx ctx;
  FILE *f;
}
fileIOCtx;

gdIOCtx *newFileCtx (FILE * f);

static int filePutbuf (gdIOCtx *, const void *, int);
static void gdFreeFileCtx (gdIOCtx * ctx);

/* return data as a dynamic pointer */
BGD_DECLARE(gdIOCtx *) gdNewFileCtx (FILE * f)
{
  fileIOCtx *ctx;

  ctx = (fileIOCtx *) gdMalloc (sizeof (fileIOCtx));
  if (ctx == NULL)
  {
      return NULL;
  }

  ctx->f = f;
  ctx->ctx.putBuf = filePutbuf;
  ctx->ctx.gd_free = gdFreeFileCtx;

  return (gdIOCtx *) ctx;
}

static void
gdFreeFileCtx (gdIOCtx * ctx)
{
  gdFree (ctx);
}

static int
filePutbuf (gdIOCtx * ctx, const void *buf, int size)
{
  fileIOCtx *fctx;
  fctx = (fileIOCtx *) ctx;

  return fwrite (buf, 1, size, fctx->f);
}
