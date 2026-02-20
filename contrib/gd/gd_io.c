/*
   * io.c
   *
   * Implements the simple I/O 'helper' routines.
   *
   * Not really essential, but these routines were used extensively in GD,
   * so they were moved here. They also make IOCtx calls look better...
   *
   * Written (or, at least, moved) 1999, Philip Warner.
   *
 */

#include "gd.h"

int
gdPutBuf (const void *buf, int size, gdIOCtx * ctx)
{
  return (ctx->putBuf) (ctx, buf, size);
}
