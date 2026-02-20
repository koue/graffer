#ifdef __cplusplus
extern "C" {
#endif

#ifndef GDHELPERS_H
#define GDHELPERS_H 1

/* These functions wrap memory management. gdFree is
	in gd.h, where callers can utilize it to correctly
	free memory allocated by these functions with the
	right version of free(). */
void *gdCalloc (size_t nmemb, size_t size);
void *gdMalloc (size_t size);
void *gdRealloc (void *ptr, size_t size);

/* Returns nonzero if multiplying the two quantities will
	result in integer overflow. Also returns nonzero if
	either quantity is negative. By Phil Knirsch based on
	netpbm fixes by Alan Cox. */

int overflow2(int a, int b);

#endif /* GDHELPERS_H */

#ifdef __cplusplus
}
#endif
