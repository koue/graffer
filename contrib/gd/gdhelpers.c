#include "gd.h"
#include <stdlib.h>

void *
gdCalloc (size_t nmemb, size_t size)
{
  return calloc (nmemb, size);
}

void *
gdMalloc (size_t size)
{
  return malloc (size);
}

void *
gdRealloc (void *ptr, size_t size)
{
  return realloc (ptr, size);
}

BGD_DECLARE(void) gdFree (void *ptr)
{
  free (ptr);
}
