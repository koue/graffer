/* Copyright (c) 2018-2023 Nikola Kolev <koue@chaosophia.net> */
/* Copyright (c) University of Cambridge 2000 - 2008 */
/* See the file NOTICE for conditions of use and distribution. */

#include "pool.h"

/* Class which provides memory allocation pools: client routines can make
 * arbitary number of allocation requests against a single pool, but all
 * the allocated memory is freed by a single pool_free() operation. Poor
 * mans garbage collection, but a whole lot easier than tracking thousands
 * of independant malloc operations, especially given the large amount of
 * temporary state which is allocated as a consequence of an incoming
 * HTTP request. You will see an awful lot of request->pool references
 * scattered around the rest of the Prayer code */

/* ====================================================================== */

/* pool_create() ***********************************************************
 *
 * Create a new memory pool.
 * blocksize:
 *   Size of aggregate allocation blocks. This is not an upper limit on the
 *   size of alloc requests against the block. It just defines the size of
 *   memory blocks which be be used to store multiple items.
 *   NULL => use a default which is sensible for many small allocation requests.
 *
 * Returns: New pool
 **************************************************************************/

struct pool *pool_create(unsigned long blocksize)
{
    struct pool *pool;

    if ((pool = (struct pool *) malloc(sizeof(struct pool))) == NULL) {
        fprintf(stderr, "pool_create(): Out of memory\n");
	exit(1);
    }

    pool->first = pool->last = NULL;
    pool->blocksize =
        (blocksize > 0) ? blocksize : PREFERRED_POOL_BLOCK_SIZE;
    pool->avail = 0;

    /* Reserve space for linked list pointer in block chain */
    if (pool->blocksize > (sizeof(struct pool_elt *)))
        pool->blocksize -= (sizeof(struct pool_elt *));

    return (pool);
}

/* pool_free() *************************************************************
 *
 * Free all memory allocated against this pool.
 *  p: Pool to free
 **************************************************************************/

void pool_free(struct pool *p)
{
    struct pool_elt *pe = p->first;
    struct pool_elt *tofree;

    while (pe) {
        tofree = pe;
        pe = pe->next;
        free(tofree);
    }
    free(p);
}

/* ====================================================================== */

/* pool_alloc() ************************************************************
 *
 * Allocate memory using given pool
 *    p: Pool
 * size: Number of bytes to allocate
 *
 * Returns: Ptr to storage space.
 **************************************************************************/

void *pool_alloc(struct pool *p, unsigned long size)
{
    struct pool_elt *pe;
    void *result;

    if (p == NULL) {
        /* Convert to simple malloc if no pool given */
        if ((result = (void *) malloc(size)) == NULL) {
            fprintf(stderr, "Out of memory\n");
	    exit(1);
	}
        return (result);
    }

    /* Round up to 8 byte boundary: Many processors expect 8 byte alignment */
    if (size > 0)
        size = size - (size % 8) + 8;

    if (size <= p->avail) {
        /* Simple case: space still available in current bucket */
        void *result = (void *) &(p->last->data[p->blocksize - p->avail]);
        p->avail -= size;
        return (result);
    }

    if (size <= p->blocksize) {
        /* Data will fit into empty normal sized bucket */
        pe = ((struct pool_elt *)
              (malloc(sizeof(struct pool_elt) + p->blocksize - 1)));

        if (pe == NULL) {
            fprintf(stderr, "Out of memory\n");
	    exit(1);
	}

        p->avail = p->blocksize - size; /* Probably some space left over */

        if (p->first) {
            /* Add to end of linked list */
            p->last->next = pe;
            p->last = pe;
        } else
            /* First element in linked list */
            p->first = p->last = pe;

        pe->next = NULL;
        return (&pe->data[0]);
    }

    /* Data too big for standard bucket: allocate oversized bucket */
    pe = ((struct pool_elt
           *) (malloc(sizeof(struct pool_elt) + size - 1)));

    if (pe == NULL) {
        fprintf(stderr, "Out of memory\n");
	exit(1);
    }

    /* We add oversized pe blocks to the _start_ of the linked list This way
     * we can continue to use partly filled buckets at the end of the
     * chain for small data items. Hopefully a bit more efficient than naive
     * allocation scheme */

    if (p->first) {
        pe->next = p->first;    /* Add pe to front of linked list */
        p->first = pe;
        /* We leave p->avail untouched: it refers to different bucket */
    } else {
        /* List was empty, need to create anyway */
        p->first = p->last = pe;
        pe->next = NULL;
        p->avail = 0;
    }

    return (&pe->data[0]);
}

/* ====================================================================== */

/* pool_strdup() ************************************************************
 *
 * Duplicate string
 *     p: Target Pool
 * value: String to duplicate
 *
 * Returns: Ptr to dupped string
 **************************************************************************/

char *pool_strdup(struct pool *p, const char *value)
{
    char *s;

    if (value == NULL)
        return (0);

    s = pool_alloc(p, strlen(value) + 1);
    strcpy(s, value);

    return (s);
}

/* pool_strcat() ************************************************************
 *
 * Concatenate two strings
 *     p: Target Pool
 *    s1: First string
 *    s2: Second string
 *
 * Returns: Ptr to combined version
 **************************************************************************/

char *pool_strcat(struct pool *p, char *s1, char *s2)
{
    char *s;

    if (!(s1 && s2))
        return (0);

    s = pool_alloc(p, strlen(s1) + strlen(s2) + 1);
    strcpy(s, s1);
    strcat(s, s2);

    return (s);
}

/* pool_strcat3() **********************************************************
 *
 * Concatenate three strings
 *     p: Target Pool
 *    s1: First string
 *    s2: Second string
 *    s3: Second string
 *
 * Returns: Ptr to combined version
 **************************************************************************/

char *pool_strcat3(struct pool *p, char *s1, char *s2, char *s3)
{
    char *s;

    if (!(s1 && s2 && s3))
        return (0);

    s = pool_alloc(p, strlen(s1) + strlen(s2) + strlen(s3) + 1);
    strcpy(s, s1);
    strcat(s, s2);
    strcat(s, s3);

    return (s);
}

/* ====================================================================== */

/* Static support routines for pool_printf() */

static unsigned long pool_ulong_size(unsigned long value)
{
    unsigned long digits = 1;

    for (value /= 10; value > 0; value /= 10)
        digits++;

    return (digits);
}

static char *pool_ulong_print(char *s, unsigned long value)
{
    unsigned long tmp, weight;

    /* All numbers contain at least one digit.
     * Find weight of most significant digit. */
    for (weight = 1, tmp = value / 10; tmp > 0; tmp /= 10)
        weight *= 10;

    for (tmp = value; weight > 0; weight /= 10) {
        if (value >= weight) {  /* Strictly speaking redundant... */
            *s++ = '0' + (value / weight);      /* Digit other than zero */
            value -= weight * (value / weight); /* Calculate remainder */
        } else
            *s++ = '0';
    }
    return (s);
}

/* ====================================================================== */

/* pool_vprintf_size() *****************************************************
 *
 * Calculate size of target string for pool_vprintf and friend
 *   fmt: vprintf format string
 *    ap: va_list
 *
 * Returns: Size in characters.
 **************************************************************************/

unsigned long pool_vprintf_size(char *fmt, va_list ap)
{
    unsigned long count = 0;
    char *s;
    char c;

    while ((c = *fmt++)) {
        if (c != '%') {
            count++;
        } else
            switch (*fmt++) {
            case 's':          /* string */
                if ((s = va_arg(ap, char *)))
                     count += strlen(s);
                else
                    count += strlen("(nil)");
                break;
            case 'l':
                if (*fmt == 'u') {
                    count += pool_ulong_size(va_arg(ap, unsigned long));
                    fmt++;
                } else
                    count += pool_ulong_size(va_arg(ap, long));
                break;
            case 'd':
                if (*fmt == 'u') {
                    count += pool_ulong_size(va_arg(ap, unsigned int));
                    fmt++;
                } else
                    count += pool_ulong_size(va_arg(ap, int));
                break;
            case 'c':
                (void) va_arg(ap, int);
                count++;
                break;
            case '%':
                count++;
                break;
            default:
                fprintf(stderr, "Bad format string to buffer_printf\n");
		exit(1);
            }
    }
    return (count);
}

/* pool_vprintf() **********************************************************
 *
 * Print va_list into (already allocated) target string.
 * target: Target area
 *   fmt:  vprintf format string
 *    ap:  va_list
 **************************************************************************/

void pool_vprintf(char *target, char *fmt, va_list ap)
{
    char *s, *d = target;
    char c;

    while ((c = *fmt++)) {
        if (c != '%') {
            *d++ = c;
        } else
            switch (*fmt++) {
            case 's':          /* string */
                if ((s = va_arg(ap, char *))) {
                    while ((c = *s++))
                        *d++ = c;
                } else {
                    strcpy(d, "(nil)");
                    d += strlen("(nil)");
                }
                break;
            case 'l':
                if (*fmt == 'u') {
                    d = pool_ulong_print(d, va_arg(ap, unsigned long));
                    fmt++;
                } else
                    d = pool_ulong_print(d, va_arg(ap, long));
                break;
            case 'd':
                if (*fmt == 'u') {
                    d = pool_ulong_print(d, va_arg(ap, unsigned int));
                    fmt++;
                } else
                    d = pool_ulong_print(d, va_arg(ap, int));
                break;
            case 'c':
                *d++ = (char) va_arg(ap, int);
                break;
            case '%':
                *d++ = '%';
                break;
            default:
                fprintf(stderr, "Bad format string to buffer_printf\n");
            }
    }
    *d = '\0';
}

/* pool_printf() ***********************************************************
 *
 * sprintf equalivant for pools: allocates space, then sprintfs into it
 *  pool: Target pool.
 *   fmt: vprintf format string, followed by arguments.
 **************************************************************************/

char *pool_printf(struct pool *p, char *fmt, ...)
{
    va_list ap;
    char *target;
    unsigned long size;

    va_start(ap, fmt);
    size = pool_vprintf_size(fmt, ap);
    va_end(ap);

    target = pool_alloc(p, size + 1);

    va_start(ap, fmt);
    pool_vprintf(target, fmt, ap);
    va_end(ap);

    return (target);
}

/* ====================================================================== */

/* pool_join() ***********************************************************
 *
 * Join an array of strings.
 *    pool: Target pool
 * join_char: Separator character e.g: '/' for filenames
 *    argv: 0 terminated array of strings to join.
 *
 * Returns: joined string
 *
 * Example: pool_join(pool, '/', &{"Hello", "World", 0}) -> "Hello/World"
 ************************************************************************/

char *pool_join(struct pool *pool, char join_char, char *argv[])
{
    char *result, *s;
    char **p;
    unsigned long len;

    for (p = argv, len = 0; *p; p++)
        len += strlen(*p) + 1;

    result = pool_alloc(pool, len);

    for (p = argv, s = result; *p; p++) {
        strcpy(s, *p);
        s += strlen(s);
        if (p[1])
            *s++ = join_char;
    }
    return (result);
}
