/* Copyright (c) 2018-2023 Nikola Kolev <koue@chaosophia.net> */
/* Copyright (c) University of Cambridge 2000 - 2008 */
/* See the file NOTICE for conditions of use and distribution. */

#ifndef _LIBPOOL_H
#define _LIBPOOL_H
#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Pool:
 *  Memory allocated in pools which can be freed as a single operation.
 *  Idea is to make memory management for transient data structures
 *  a bit easier. Does use more memory than direct malloc/free.
 *
 * Methods:
 *   pool_create (initialise pool data structure)
 *   pool_alloc  (allocate memory using nominated pool)
 *   pool_free   (free all memory allocated in this pool).
 */

/* Malloc pool in 4Kbyte chunks by default */
#define PREFERRED_POOL_BLOCK_SIZE  (4096)

struct pool_elt {
    struct pool_elt *next;      /* Linked list of data blocks */
    char data[1];
};

struct pool {
    struct pool_elt *first;     /* First data block */
    struct pool_elt *last;      /* Last data block */
    unsigned long blocksize;    /* Default block size for small allocations */
    unsigned long avail;        /* Available space in current (last) block */
};

struct pool *pool_create(unsigned long blocksize);
void pool_free(struct pool *p);
void *pool_alloc(struct pool *p, unsigned long size);
char *pool_strdup(struct pool *p, const char *value);
char *pool_strcat(struct pool *p, char *s1, char *s2);
char *pool_strcat3(struct pool *p, char *s1, char *s2, char *s3);

unsigned long pool_vprintf_size(char *fmt, va_list ap);
void pool_vprintf(char *target, char *fmt, va_list ap);
char *pool_printf(struct pool *p, char *fmt, ...);

char *pool_join(struct pool *pool, char join_char, char *argv[]);

#endif

