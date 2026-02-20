#ifdef __cplusplus
extern "C" {
#endif

#ifndef GD_IO_H
#define GD_IO_H 1

#include <stdio.h>

typedef struct gdIOCtx
{
  int (*putBuf) (struct gdIOCtx *, const void *, int);
  void (*gd_free) (struct gdIOCtx *);
}
gdIOCtx;

typedef struct gdIOCtx *gdIOCtxPtr;
int gdPutBuf (const void *, int, gdIOCtx *);

#endif

#ifdef __cplusplus
}
#endif
