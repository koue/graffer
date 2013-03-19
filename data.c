/* $Id: data.c,v 1.3 2010/05/27 10:22:38 dhartmei Exp $ */

/*
 * Copyright (c) 2002-2010, Daniel Hartmeier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer. 
 *    - Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

static const char rcsid[] = "$Id: data.c,v 1.3 2010/05/27 10:22:38 dhartmei Exp $";

#include <sys/types.h>
#include <netinet/in.h>
#include <db.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "data.h"

struct key {
	u_int16_t	 unit;
	u_int16_t	 level;
	u_int32_t	 ts;
};

struct val {
	double		 min;
	double		 avg;
	double		 max;
};

struct last {
	unsigned	 since;
	unsigned	 ts;
	double		 val;
};

extern int		 debug;
static const char	*fn = NULL;
static BTREEINFO	 btreeinfo;
static DB		*db;
static DBT		 dbk, dbd;
static struct key	 k;
static struct val	 v;
static struct last	 l;

#define	MAX_LEVEL	((u_int16_t)0xffffU)
#define	MAX_TS		((u_int32_t)0xffffffffU)

static short
find_highest_level(unsigned short unit)
{
	int r;

	memset(&k, 0, sizeof(k));
	k.unit = htons(unit);
	k.level = htons(MAX_LEVEL - 1);
	memset(&dbk, 0, sizeof(dbk));
	dbk.size = sizeof(k);
	dbk.data = &k;
	r = db->seq(db, &dbk, &dbd, R_CURSOR);
	if (!r)
		r = db->seq(db, &dbk, &dbd, R_PREV);
	else
		r = db->seq(db, &dbk, &dbd, R_LAST);
	if (r || dbk.size != sizeof(k) || !dbk.data)
		return (0);
	memcpy(&k, dbk.data, sizeof(k));
	k.unit = ntohs(k.unit);
	if (k.unit != unit)
		return (0);
	k.level = ntohs(k.level);
	if (debug > 0)
		printf("find_highest_level(unit %d) returning level %d\n",
		    (int)unit, (int)k.level);
	return (k.level);
}

static unsigned
find_highest_ts(unsigned short unit, short level)
{
	int r;

	memset(&k, 0, sizeof(k));
	k.unit = htons(unit);
	k.level = htons(level);
	k.ts = htonl(MAX_TS);
	memset(&dbk, 0, sizeof(dbk));
	dbk.size = sizeof(k);
	dbk.data = &k;
	if (debug > 1)
		printf("find_highest_ts(unit %d, level %d) seeking\n",
		    (int)unit, (int)level);
	r = db->seq(db, &dbk, &dbd, R_CURSOR);
	if (!r)
		r = db->seq(db, &dbk, &dbd, R_PREV);
	else
		r = db->seq(db, &dbk, &dbd, R_LAST);
	if (r || dbk.size != sizeof(k) || !dbk.data) {
		if (debug > 1)
			printf("find_highest_ts: seek failed, returning 0\n");
		return (0);
	}
	memcpy(&k, dbk.data, sizeof(k));
	k.unit = ntohs(k.unit);
	k.level = ntohs(k.level);
	if (k.unit != unit || k.level != level) {
		if (debug > 1)
			printf("find_highest_ts: out of range "
			    "(unit %d != %d, level %d != %d), returning 0\n",
			    (int)k.unit, (int)unit, (int)k.level, (int)level);
		return (0);
	}
	k.ts = ntohl(k.ts);
	if (debug > 0)
		printf("find_highest_ts(unit %d, level %d) returning ts %u\n",
		     (int)unit, (int)level, (unsigned)k.ts);
	return ((unsigned)k.ts);
}

static unsigned
count_values(unsigned short unit, short level, unsigned ts,
    double *min, double *avg, double *max)
{
	unsigned count = 0;
	int r;
	unsigned tsp = ts;
	double avgp = 0.0;

	*min = DBL_MAX;
	*max = -DBL_MAX;
	*avg = 0.0;

	memset(&k, 0, sizeof(k));
	k.unit = htons(unit);
	k.level = htons(level);
	k.ts = htonl(ts);

	memset(&dbk, 0, sizeof(dbk));
	dbk.size = sizeof(k);
	dbk.data = &k;

	memset(&dbd, 0, sizeof(dbd));

	for (r = db->seq(db, &dbk, &dbd, R_CURSOR); !r;
	    r = db->seq(db, &dbk, &dbd, R_NEXT)) {
		if (dbk.size != sizeof(k) || !dbk.data)
			break;
		memcpy(&k, dbk.data, sizeof(k));
		k.unit = ntohs(k.unit);
		k.level = ntohs(k.level);
		k.ts = ntohl(k.ts);
		if (k.unit != unit || k.level != level)
			break;
		if (dbd.size != sizeof(v) || !dbd.data)
			break;
		memcpy(&v, dbd.data, sizeof(v));

		if (!ts)
			ts = k.ts;
		if (v.min < *min)
			*min = v.min;
		if (v.max > *max)
			*max = v.max;
		*avg += avgp * (k.ts - tsp);
		avgp = v.avg;
		tsp = k.ts;
		count++;
	}
	if (count && tsp > ts)
		*avg /= (tsp - ts);
	if (debug > 1)
		printf("count_values(unit %d, level %d, ts %u) returning "
		    "count %u\n", (int)unit, (int)level, ts, count);
	return (count);
}

static int
put_value_internal(unsigned short unit, short level, unsigned ts,
    double min, double avg, double max)
{
	unsigned max_ts, count;

	if (debug > 0)
		printf("put_value_internal(unit %d, level %d, ts %u, min %.2f, "
		    "avg %.2f, max %.2f)\n", (int)unit, (int)level, ts, min,
		    avg, max);

	memset(&k, 0, sizeof(k));
	k.unit = htons(unit);
	k.level = htons(level);
	k.ts = htonl(ts);
	memset(&dbk, 0, sizeof(dbk));
	dbk.size = sizeof(k);
	dbk.data = &k;

	memset(&v, 0, sizeof(v));
	v.min = min;
	v.avg = avg;
	v.max = max;
	memset(&dbd, 0, sizeof(dbd));
	dbd.size = sizeof(v);
	dbd.data = &v;

	if (db->put(db, &dbk, &dbd, 0)) {
		fprintf(stderr, "db->put: %s\n", strerror(errno));
		return (1);
	}

	if (debug > 1)
		printf("put_value_internal: inserted, checking next level %d\n",
		    (int)(level + 1));
	max_ts = find_highest_ts(unit, level + 1);
	if (debug > 1)
		printf("put_value_internal: next level %d max_ts %u\n",
		    (int)(level + 1), max_ts);
	count = count_values(unit, level, max_ts, &min, &avg, &max);
	if (debug > 1)
		printf("put_value_internal: %u values on level %d since %u\n",
		    count, (int)level, max_ts);
	if (count < 10) {
		if (debug > 1)
			printf("put_value_internal: count %u < 10\n", count);
		return (0);
	}
	if (debug > 1)
		printf("put_value_internal: count %u >= 10\n", count);
	return (put_value_internal(unit, level + 1, ts, min, avg, max));
}

static int
get_last(unsigned short unit, unsigned *since, unsigned *ts, double *val)
{
	int r;

	memset(&k, 0, sizeof(k));
	k.unit = htons(unit);
	k.level = htons(MAX_LEVEL);
	k.ts = htonl(0);
	memset(&dbk, 0, sizeof(dbk));
	dbk.size = sizeof(k);
	dbk.data = &k;
	memset(&dbd, 0, sizeof(dbd));
	r = db->get(db, &dbk, &dbd, 0);
	if (r > 0)
		/* key not found */
		return (1);
	if (r < 0) {
		fprintf(stderr, "db->get: %s\n", strerror(errno));
		return (1);
	}
	if (dbd.size != sizeof(l) || !dbd.data)
		return (1);
	memcpy(&l, dbd.data, sizeof(l));
	*since = l.since;
	*ts = l.ts;
	*val = l.val;
	if (debug > 0)
		printf("get_last(unit %u) returns since %u, ts %u, val %.2f\n",
		    (unsigned)unit, *since, *ts, *val);
	return (0);
}

static int
put_last(unsigned short unit, unsigned since, unsigned ts, double val)
{
	if (debug > 0)
		printf("put_last(unit %u, since %u, ts %u, val %.2f)\n",
		    (unsigned)unit, since, ts, val);
	memset(&k, 0, sizeof(k));
	k.unit = htons(unit);
	k.level = htons(MAX_LEVEL);
	k.ts = htonl(0);
	memset(&dbk, 0, sizeof(dbk));
	dbk.size = sizeof(k);
	dbk.data = &k;
	memset(&l, 0, sizeof(l));
	l.since = since;
	l.ts = ts;
	l.val = val;
	memset(&dbd, 0, sizeof(dbd));
	dbd.size = sizeof(l);
	dbd.data = &l;
	if (db->put(db, &dbk, &dbd, 0)) {
		fprintf(stderr, "db->put: %s\n", strerror(errno));
		return (1);
	}
	return (0);
}

int
data_put_value(unsigned since, unsigned ts, unsigned short unit, double val,
    int diff)
{
	if (debug > 0)
		printf("data_put_value(since %u, ts %u, unit %u, val %.2f, "
		    "diff %d)\n", since, ts, (unsigned)unit, val, diff);
	if (diff) {
		/* find previous value and ts, calculate diff per second */
		int skip = 1;
		unsigned last_since, last_ts;
		double last_val;

		if (!get_last(unit, &last_since, &last_ts, &last_val) &&
		    last_since == since && last_ts < ts && last_val <= val)
			skip = 0;
		put_last(unit, since, ts, val);
		if (skip)
			return (0);
		val = (val - last_val) / (ts - last_ts);
	}
	return (put_value_internal(unit, 0, ts, val, val, val));
}

/* find highest level of unit with more than siz entries within beg-end */
static int
get_values_find_level(unsigned short unit, unsigned beg, unsigned end,
    unsigned siz)
{
	short level = 0;

	/* find highest level at all */
	level = find_highest_level(unit);
	if (debug > 0)
		printf("get_values_find_level: highest level overall is %d\n",
		    (int)level);
	while (level > 0) {
		unsigned count = 0;
		int r;

		if (debug > 0)
			printf("get_values_find_level: trying level %d\n",
			    (int)level);

		memset(&k, 0, sizeof(k));
		k.unit = htons(unit);
		k.level = htons(level);
		k.ts = htonl(beg);
		memset(&dbk, 0, sizeof(dbk));
		dbk.size = sizeof(k);
		dbk.data = &k;

		for (r = db->seq(db, &dbk, &dbd, R_CURSOR); !r;
		    r = db->seq(db, &dbk, &dbd, R_NEXT)) {
			if (dbk.size != sizeof(k) || !dbk.data)
				break;
			memcpy(&k, dbk.data, sizeof(k));
			k.unit = ntohs(k.unit);
			k.level = ntohs(k.level);
			k.ts = ntohs(k.ts);
			if (k.unit != unit || k.level != level || k.ts > end)
				break;
			++count;
		}
		if (debug > 0)
			printf("get_values_find_level: found %u entries "
			    "for level %d\n", count, (int)level);
		if (siz + 32 <= count) {
			if (debug > 0)
				printf("get_values_find_level: siz %u + 32 "
				    "<= count %u, using level %d\n",
				    siz, count, (int)level);
			break;
		}
		--level;
	}
	if (debug > 0)
		printf("get_values_find_level(unit %d, beg %u, end %u, siz %u) "
		    "returning level %d\n", (int)unit, beg, end, siz, level);
	return (level);
}

static inline double
intersect(double sa, double sb, double pa, double pb)
{
	/* sa <= sb && pa <= pb */
	if (sb <= pa || sa >= pb)
		return 0.0;
	if (sa <= pa && sb >= pb)
		return pb - pa;
	if (sa >= pa && sb <= pb)
		return sb - sa;
	if (sa <= pa)
		return sb - pa;
	else
		return pb - sa;
}

static inline void
get_values_resample(unsigned beg, unsigned end, int type, unsigned siz,
    double *a, double spp, unsigned tt, unsigned ts, double v)
{
	unsigned i = (unsigned long long)(tt - beg) * siz / (end - beg);
	unsigned j = (unsigned long long)(ts - beg) * siz / (end - beg);
	double sa = (double)tt - (double)beg;
	double sb = (double)ts - (double)beg;
	double pa, f;   

	if (debug > 2)
		printf("get_values_resample(beg %u, end %u, type %d, siz %u, "
		    "spp %.2f, tt %u, ts %u, v %.2f)\n",
		    beg, end, type, siz, spp, tt, ts, v);
	if (debug > 2)
		printf("get_values_resample: enter, i %u, j %u\n", i, j);
	while (i <= j) {
		if (debug > 2)
			printf("get_values_resample: i %u <= j %u\n", i, j);
		pa = (double)i * spp;
		if ((f = intersect(sa, sb, pa, pa + spp)) > 0.0) {
			switch (type) {
			case DATA_TYPE_AVG:
				a[i] += v * (f / spp);
				break;
			case DATA_TYPE_MAX:
				if (a[i] < v)
					a[i] = v;
				break;
			case DATA_TYPE_MIN:
				if (a[i] > v)
					a[i] = v;
				break;
			}
		}
		++i;
	}
}

int
data_get_values(unsigned short unit, unsigned beg, unsigned end, int type,
    unsigned siz, double *a)
{
	double spp, d;
	unsigned i, ts, tt = 0;
	int level, r;

	if (beg >= end) {
		fprintf(stderr, "get_values: beg %u >= end %u\n", beg, end);
		return (1);
	}
	if (type != DATA_TYPE_MIN && type != DATA_TYPE_AVG &&
	    type != DATA_TYPE_MAX) {
		fprintf(stderr, "get_values: invalid type %d\n", type);
		return (1);
	}
	spp = (double)(end - beg) / (double)siz;
	for (i = 0; i < siz; ++i)
		a[i] = type == DATA_TYPE_AVG ? 0.0 :
		    (type == DATA_TYPE_MAX ? -DBL_MAX : DBL_MAX);
	level = get_values_find_level(unit, beg, end, siz);
	if (level < 0)
		return (1);

	if (debug > 1)
		printf("get_values: seeking for %d, %d, %u\n", (int)unit,
		    (int)level, end);

	memset(&k, 0, sizeof(k));
	k.unit = htons(unit);
	k.level = htons(level);
	k.ts = htonl(end);
	memset(&dbk, 0, sizeof(dbk));
	dbk.size = sizeof(k);
	dbk.data = &k;

	r = db->seq(db, &dbk, &dbd, R_CURSOR);
	if (!r) {
		if (dbk.size != sizeof(k) || !dbk.data)
			return (1);
		memcpy(&k, dbk.data, sizeof(k));
		k.unit = ntohs(k.unit);
		k.level = ntohs(k.level);
		k.ts = ntohl(k.ts);
		if (debug > 1)
			printf("get_values: first %d, %d, %u\n", (int)k.unit,
			    (int)k.level, k.ts);
		if (k.unit != unit || k.level != level) {
			if (debug > 1)
				printf("get_values: first past end, prev\n");
			r = db->seq(db, &dbk, &dbd, R_PREV);
		}
	}
	for (; !r; r = db->seq(db, &dbk, &dbd, R_PREV)) {
		if (dbk.size != sizeof(k) || !dbk.data)
			break;
		memcpy(&k, dbk.data, sizeof(k));
		k.unit = ntohs(k.unit);
		k.level = ntohs(k.level);
		k.ts = ntohl(k.ts);
		if (debug > 1)
			printf("get_values: got %d, %d, %u\n",
			    (int)k.unit, (int)k.level, k.ts);
		if (k.unit != unit || k.level != level || k.ts < beg) {
			if (debug > 0)
				printf("get_values: end of sequence\n");
			break;
		}
		ts = k.ts;
		if (dbd.size != sizeof(v) || !dbd.data)
			break;
		memcpy(&v, dbd.data, sizeof(v));
		if (type == DATA_TYPE_MIN)
			d = v.min;
		else if (type == DATA_TYPE_AVG)
			d = v.avg;
		else
			d = v.max;

		if (debug > 1)
			printf("get_values: tt %u, ts %u, diff %u, v %.2f\n",
			    tt, ts, tt - ts, d);
		get_values_resample(beg, end, type, siz, a, spp,
		    ts >= beg ? ts : beg, tt ? tt : end, d);
		tt = ts;
	}

	for (i = 0; i < siz; ++i)
		if (a[i] <= -DBL_MAX || a[i] >= DBL_MAX)
			a[i] = 0.0;
	if (debug) {
		d = -DBL_MAX;
		for (i = 0; i < siz; ++i)
			if (a[i] > d)
				d = a[i];
		if (debug > 0)
			printf("get_values: maximum (%u values) %.2f\n",
			    siz, d);
	}
	return (0);
}

int
data_open(const char *filename)
{
	fn = filename;
	memset(&btreeinfo, 0, sizeof(btreeinfo));
	db = dbopen(fn, O_CREAT|O_EXLOCK|O_RDWR, 0600,
	    DB_BTREE, &btreeinfo);
	if (db == NULL) {
		fprintf(stderr, "dbopen: %s: %s\n", fn, strerror(errno));
		return (1);
	}
	return (0);
}

int
data_close()
{
	if (db->sync(db, 0))
		fprintf(stderr, "dbsync: %s: %s\n", fn, strerror(errno));
	if (db->close(db))
		fprintf(stderr, "dbclose: %s: %s\n", fn, strerror(errno));
	return (0);
}

int
data_truncate(unsigned days_detail, unsigned days_compressed)
{
	int r;
	unsigned cutoff[2];
	unsigned seen = 0, deleted = 0;

	cutoff[0] = time(NULL) - days_detail * 24 * 60 * 60;
	cutoff[1] = time(NULL) - days_compressed * 24 * 60 * 60;
	if (debug > 1)
		printf("data_truncate: cutoff %u, %u\n", cutoff[0], cutoff[1]);
	r = db->seq(db, &dbk, &dbd, R_FIRST);
	if (r < 0) 
		fprintf(stderr, "data_truncate: db->seq(R_FIRST) failed: %s\n",
		    strerror(errno));
	while (!r) {
		seen++;
		if (dbk.size != sizeof(k) || !dbk.data) {
			fprintf(stderr, "data_truncate: dbk.size %u != "
			    "sizeof(k) %u\n", (unsigned)dbk.size,
			    (unsigned)sizeof(k));
			goto delete;
		}
		memcpy(&k, dbk.data, sizeof(k));
		k.unit = ntohs(k.unit);
		k.level = ntohs(k.level);
		k.ts = ntohl(k.ts);
		if (k.level == MAX_LEVEL) {
			if (dbd.size != sizeof(l) || !dbd.data) {
				fprintf(stderr, "data_truncate: dbd.size %u "
				    "!= sizeof(l) %u\n", (unsigned)dbd.size,
				    (unsigned)sizeof(l));
				goto delete;
			}
			memcpy(&l, dbd.data, sizeof(l));
			if (debug > 1)
				printf("%d, %d, %u, last: %u, %u, %.2f\n",
				    (int)k.unit, (int)k.level, (unsigned)k.ts,
				    l.since, l.ts, l.val);
			if (l.ts < cutoff[0])
				goto delete;
			else
				goto next;
		} else {
			if (dbd.size != sizeof(v) || !dbd.data) {
				fprintf(stderr, "data_truncate: dbd.size %u "
				    "!= sizeof(v) %u\n", (unsigned)dbd.size,
				    (unsigned)sizeof(v));
				goto delete;
			}
			memcpy(&v, dbd.data, sizeof(v));
			if (debug > 1)
				printf("%d, %d, %u, val: %.2f, %.2f, %.2f\n",
				    (int)k.unit, (int)k.level, (unsigned)k.ts,
				    v.min, v.avg, v.max);
			if (k.ts < cutoff[k.level > 0])
				goto delete;
			else
				goto next;
		}
delete:
		r = db->del(db, &dbk, 0);
		if (r < 0) {
			fprintf(stderr, "data_truncate: db->del() failed: %s\n",
			    strerror(errno));
			return (1);
		}
		if (r > 0) {
			fprintf(stderr, "data_truncate: db->del() "
			    "returned %d\n", r);
			return (1);
		}
		deleted++;
next:
		r = db->seq(db, &dbk, &dbd, R_NEXT);
		if (r < 0)
			fprintf(stderr, "db->seq(R_NEXT) failed: %s\n",
			    strerror(errno));
	}
	if (debug > 0)
		printf("data_truncate: %u of %u entries deleted\n",
		    deleted, seen);
	return (0);
}

int
data_copy(const char *filename)
{
	BTREEINFO bti2;
	DB *db2;
	int r;
	unsigned count = 0;

	if (debug > 0)
		printf("data_copy: creating %s\n", filename);
	memset(&bti2, 0, sizeof(bti2));
	db2 = dbopen(filename, O_CREAT|O_EXLOCK|O_RDWR, 0600, DB_BTREE, &bti2);
	if (db2 == NULL) {
		fprintf(stderr, "data_copy: dbopen: %s: %s\n", filename,
		    strerror(errno));
		return (1);
	}

	r = db->seq(db, &dbk, &dbd, R_FIRST);
	if (r < 0) {
		fprintf(stderr, "data_copy: db->seq(R_FIRST) failed: %s\n",
		    strerror(errno));
		return (1);
	}
	do {
		if (dbk.data == NULL || dbk.size != sizeof(k)) {
			fprintf(stderr, "data_copy: invalid record: "
			    "dbk.size %u (%u)\n", (unsigned)dbk.size,
			    (unsigned)sizeof(k));
		} else if (dbd.data == NULL || dbd.size !=
		    (ntohs(((struct key *)dbk.data)->level) == MAX_LEVEL ?
		    sizeof(l) : sizeof(v))) {
			fprintf(stderr, "data_copy: invalid record: level %u, "
			    "dbd.size %u (%u, %u)\n",
			    (unsigned)ntohs(((struct key *)dbk.data)->level),
			    (unsigned)dbd.size, (unsigned)sizeof(l),
			    (unsigned)sizeof(v));
		} else if (db2->put(db2, &dbk, &dbd, 0)) {
			fprintf(stderr, "data_copy: db->put: %s\n",
			    strerror(errno));
			break;
		} else {
			count++;
			if (debug > 1 && count % 10000 == 0)
				printf(" %u", count);
		}
		r = db->seq(db, &dbk, &dbd, R_NEXT);
		if (r < 0)
			fprintf(stderr, "data_copy: db->seq(R_NEXT) failed: "
			    "%s\n", strerror(errno));
	} while (!r);
	if (debug > 1)
		printf("\n");

	if (db2->sync(db2, 0))
		fprintf(stderr, "data_copy: dbsync: %s: %s\n", filename,
		    strerror(errno));
	if (db2->close(db2))
		fprintf(stderr, "data_copy: dbclose: %s: %s\n", filename,
		    strerror(errno));
	if (debug > 0)
		printf("data_copy: %u records copied\n", count);
	return (0);
}
