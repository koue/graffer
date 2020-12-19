/*
 * Copyright (c) 2002-2010, Daniel Hartmeier
 * Copyright (c) 2013, Nikola Kolev
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "data.h"
#include "graph.h"

extern int	 parse_config(const char *, struct matrix **);

struct col {
	unsigned	 nr;
	char		 arg[128];
	int		 diff;
	double		 val;
} cols[512];

unsigned maxcol = 0;
unsigned since = 0;
int debug = 0;

int
add_col(unsigned nr, const char *arg, int diff)
{
	int i;

	for (i = 0; i < maxcol; ++i) {
		if (cols[i].nr == nr) {
			fprintf(stderr, "add_col: %d already defined\n", nr);
			return (1);
		}
	}
	if (maxcol == sizeof(cols) / sizeof(cols[0])) {
		fprintf(stderr, "add_col: limit of %d collects reached\n",
		    maxcol);
		return (1);
	}
	cols[maxcol].nr = nr;
	strlcpy(cols[maxcol].arg, arg, sizeof(cols[maxcol].arg));
	cols[maxcol].diff = diff;
	maxcol++;
	return (0);
}

double
value_query(const char *arg) {
	
	char query_path[256], result[256], *end;
	FILE *fp;

	if(debug)
		printf("value_query - arg [%s]\n", arg);
	snprintf(query_path, sizeof(query_path), "%s", arg);
	if(debug)
		printf("value_query - query_path [%s]\n", query_path);
	if ((fp = popen(query_path, "r")) == NULL)
		return 0;
	fgets(result, sizeof(result), fp);
	pclose(fp);
	return strtod(result, &end);
}

static void
set_col(unsigned nr, double val)
{
	int i;

	for (i = 0; i < maxcol; ++i) {
		if (cols[i].nr == nr)
			cols[i].val = val;
	}
}

static void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "usage: %s [-v] [-c config] [-d data] "
	    "[-p] [-q] [-t days[:days]] [-f file]\n", __progname);
	exit(1);
}

int
main(int argc, char *argv[])
{
	const char *configfn = "/etc/graffer/graffer.conf";
	const char *datafn = "/var/db/graffer.db";
	const char *fixfn = NULL;
	int ch, query = 0, draw = 0, trunc = 0, i;
	int days[2] = { 31, 365 };
	struct matrix *matrices = NULL, *m;
	struct graph *g;

	while ((ch = getopt(argc, argv, "c:d:f:pqt:v")) != -1) {
		switch (ch) {
		case 'c':
			configfn = optarg;
			break;
		case 'd':
			datafn = optarg;
			break;
		case 'f':
			fixfn = optarg;
			break;
		case 'p':
			draw = 1;
			break;
		case 'q':
			query = 1;
			break;
		case 't': {
			char *o, *p;

			o = strdup(optarg);
			if (!o) {
				fprintf(stderr, "main: strdup: %s\n",
				    strerror(errno));
				return (1);
			}
			p = strchr(o, ':');
			if (p != NULL) {
				*p = 0;
				days[1] = atoi(p + 1);
			}
			days[0] = atoi(o);
			if (days[0] <= 0 || days[1] <= 0)
				usage();
			trunc = 1;
			break;
		}
		case 'v':
			debug++;
			break;
		default:
			usage();
		}
	}
	if (argc != optind)
		usage();
	if (!query && !draw && !trunc && !fixfn)
		usage();

	if (parse_config(configfn, &matrices))
		return (1);

	if (data_open(datafn))
		return (1);

	if (trunc) {

		if (debug)
			printf("truncating database\n");
		if (data_truncate(days[0], days[1])) {
			fprintf(stderr, "main: data_truncate() failed\n");
			return (1);
		}

	}

	if (query) {

		if (debug)
			printf("querying values\n");

		for (i = 0; i < maxcol; ++i) {
			if (debug)
				printf("set_col(%d, %s, %lf)\n", cols[i].nr, cols[i].arg,  value_query(cols[i].arg));
			set_col(cols[i].nr, value_query(cols[i].arg));
		}

		if (debug)
			printf("storing values in database\n");
		for (i = 0; i < maxcol; ++i)
			if (data_put_value(since, time(NULL), cols[i].nr,
			    cols[i].val, cols[i].diff)) {
				fprintf(stderr, "main: data_put_value() "
				    "failed\n");
				return (1);
			}

	}

	if (draw) {

		if (debug)
			printf("generating images\n");
		for (m = matrices; m != NULL; m = m->next)
			for (i = 0; i < 2; ++i)
				for (g = m->graphs[i]; g != NULL; g = g->next) {
					if (debug)
						printf("fetching values for "
						    "unit %u from database\n",
						    g->desc_nr);
					if (data_get_values(g->desc_nr, m->beg,
					    m->end, g->type, m->w0, g->data)) {
						fprintf(stderr, "main: "
						    "data_get_values() "
						    "failed\n");
						return (1);
					}
				}
		if (debug)
			printf("drawing and writing images\n");
		if (graph_generate_images(matrices)) {
			fprintf(stderr, "main: graph_generate_images() "
			    "failed\n");
			return (1);
		}

	}

	if (fixfn) {
		if (debug)
			printf("fixing database %s to %s\n", datafn, fixfn);
		if (data_copy(fixfn)) {
			fprintf(stderr, "main: data_copy() failed\n");
			return (1);
		}
	}

	data_close();
	return (0);
}
