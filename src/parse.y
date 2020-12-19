/*
 * Copyright (c) 2002-2006 Daniel Hartmeier
 * Copyright (c) 2013 Nikola Kolev
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

%{

#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <err.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "data.h"
#include "graph.h"

extern int add_col(unsigned nr, const char *arg, int diff);

static const char *infile = NULL;
static struct matrix **matrices = NULL;
static FILE *fin = NULL;
static int debug = 0;
static int lineno = 1;
static int errors = 0;

int	 yyerror(char *, ...);
int	 yyparse();

struct node_graph {
	struct graph		 graph;
	struct node_graph	*next;
};

typedef struct {
	union {
		int			 number;
		char			*string;
		struct {
			int		 beg;
			int		 end;
		}			 time;
		struct {
			int		 width;
			int		 height;
		}			 size;
		struct {
			struct node_graph	*graph;
		}			 side;
		struct node_graph	*graph;
		struct {
			int		 type;
			char		*arg;
			int		 idx;
			int		 diff;
		}			 col;
	} v;
	int lineno;
} YYSTYPE;

%}

%token	ERROR IMAGE TIME MINUTES HOURS DAYS WEEKS MONTHS YEARS TO NOW
%token	WIDTH HEIGHT LEFT RIGHT GRAPH COLOR FILLED TYPE JPEG PNG
%token	COLLECT DIFF BPS AVG MIN MAX
%token	<v.string>	STRING
%token	<v.number>	NUMBER
%type	<v.time>	timerange
%type	<v.size>	size
%type	<v.number>	type
%type	<v.side>	left right
%type	<v.graph>	graph_item graph_list
%type	<v.number>	time filled diff bps avg
%%

configuration	: /* empty */
		| configuration collect
		| configuration image
		| configuration error		{ errors++; }
		;

collect		: COLLECT NUMBER '=' STRING diff
		{
			if (add_col($2, $4, $5)) {
				yyerror("add_col() failed");
				YYERROR;
			}
		}
		;

diff		: /* empty */		{ $$ = 0; }
		| DIFF			{ $$ = 1; }
		;

image		: IMAGE STRING '{' timerange type size left right '}'
		{
			struct node_graph *g, *h;

			if ($7.graph == NULL && $8.graph == NULL) {
				yyerror("neither left nor right graph defined");
				YYERROR;
			}
			graph_add_matrix(matrices, $2, $5, $6.width, $6.height,
			    $4.beg, $4.end);
			g = $7.graph;
			while (g != NULL) {
				graph_add_graph(&(*matrices)->graphs[0],
				    (*matrices)->w0, g->graph.desc_nr,
				    g->graph.label, g->graph.unit,
				    g->graph.color, g->graph.filled,
				    g->graph.bytes, g->graph.type);
				h = g;
				g = g->next;
				free(h->graph.label);
				free(h);
			}
			g = $8.graph;
			while (g != NULL) {
				graph_add_graph(&(*matrices)->graphs[1],
				    (*matrices)->w0, g->graph.desc_nr,
				    g->graph.label, g->graph.unit,
				    g->graph.color, g->graph.filled,
				    g->graph.bytes, g->graph.type);
				h = g;
				g = g->next;
				free(h->graph.label);
				free(h);
			}
		}
		;

timerange	: /* empty */			{
			$$.end = time(0);
			$$.beg = $$.end - 24 * 60;
		}
		| TIME time			{
			$$.end = time(0);
			$$.beg = $2;
		}
		| TIME time TO time		{
			$$.end = $4;
			$$.beg = $2;
		}
		;

time		: NOW				{
			$$ = time(0);
		}
		| NUMBER			{
			$$ = $1;
		}
		| NUMBER MINUTES		{
			$$ = time(0) - $1 * 60;
		}
		| NUMBER HOURS			{
			$$ = time(0) - $1 * 60 * 60;
		}
		| NUMBER DAYS			{
			$$ = time(0) - $1 * 60 * 60 * 24;
		}
		| NUMBER WEEKS			{
			$$ = time(0) - $1 * 60 * 60 * 24 * 7;
		}
		| NUMBER MONTHS			{
			$$ = time(0)-  $1 * 60 * 60 * 24 * 30;
		}
		| NUMBER YEARS			{
			$$ = time(0)-  $1 * 60 * 60 * 24 * 365;
		}
		;

size		: /* empty */			{
			$$.width = 640;
			$$.height = 320;
		}
		| WIDTH NUMBER HEIGHT NUMBER	{
			$$.width = $2;
			$$.height = $4;
		}
		;

type		: /* empty */			{ $$ = 0; }
		| TYPE JPEG			{ $$ = 0; }
		| TYPE PNG 			{ $$ = 1; }
		;

left		: /* empty */			{ $$.graph = NULL; }
		| LEFT graph_list		{ $$.graph = $2; }
		;

right		: /* empty */			{ $$.graph = NULL; }
		| RIGHT graph_list		{ $$.graph = $2; }
		;

graph_list	: graph_item			{ $$ = $1; }
		| graph_list ',' graph_item	{ $3->next = $1; $$ = $3; }
		;

graph_item	: GRAPH NUMBER bps avg STRING STRING COLOR NUMBER NUMBER NUMBER filled
		{
			$$ = calloc(1, sizeof(struct node_graph));
			if ($$ == NULL)
				err(1, "graph_item: calloc");
			$$->graph.desc_nr = $2;
			$$->graph.bytes = $3;
			$$->graph.type = $4;
			$$->graph.label = strdup($5);
			$$->graph.unit = strdup($6);
			if ($8 < 0 || $8 > 255 || $9 < 0 || $9 > 255 ||
			    $10 < 0 || $10 > 255) {
				yyerror("invalid color %d %d %d", $8, $9, $10);
				YYERROR;
			}
			$$->graph.color = $8 << 16 | $9 << 8 | $10;
			$$->graph.filled = $11;
		}
		;

avg		: /* empty */			{ $$ = DATA_TYPE_AVG; }
		| AVG				{ $$ = DATA_TYPE_AVG; }
		| MIN				{ $$ = DATA_TYPE_MIN; }
		| MAX				{ $$ = DATA_TYPE_MAX; }
		;

filled		: /* empty */			{ $$ = 0; }
		| FILLED			{ $$ = 1; }
		;

bps		: /* empty */			{ $$ = 0; }
		| BPS				{ $$ = 1; }
		;

%%

int
yyerror(char *fmt, ...)
{
	va_list ap;
	errors = 1;

	va_start(ap, fmt);
	fprintf(stderr, "%s:%d: ", infile, yylval.lineno);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	return (0);
}

struct keywords {
	const char	*k_name;
	int		 k_val;
};

int
kw_cmp(const void *k, const void *e)
{
	return (strcmp(k, ((struct keywords *)e)->k_name));
}

int
lookup(char *s)
{
	/* this has to be sorted always */
	static const struct keywords keywords[] = {
		{ "avg",	AVG },
		{ "bps",	BPS },
		{ "collect",	COLLECT },
		{ "color",	COLOR },
		{ "days",	DAYS },
		{ "diff",	DIFF },
		{ "filled",	FILLED },
		{ "from",	TIME },
		{ "graph",	GRAPH },
		{ "height",	HEIGHT },
		{ "hours",	HOURS },
		{ "image",	IMAGE },
		{ "jpeg",	JPEG },
		{ "left",	LEFT },
		{ "max",	MAX },
		{ "min",	MIN },
		{ "minutes",	MINUTES },
		{ "months",	MONTHS },
		{ "now",	NOW },
		{ "png",	PNG },
		{ "right",	RIGHT },
		{ "to",		TO },
		{ "type",	TYPE },
		{ "weeks",	WEEKS },
		{ "width",	WIDTH },
		{ "years",	YEARS },
	};
	const struct keywords *p;

	p = bsearch(s, keywords, sizeof(keywords)/sizeof(keywords[0]),
	    sizeof(keywords[0]), kw_cmp);

	if (p)
		return (p->k_val);
	else
		return (STRING);
}

char	*parsebuf;
int	parseindex;

int
lgetc(FILE *fin)
{
	int c, next;

restart:
	if (parsebuf) {
		/* Reading characters from the parse buffer, instead of input */
		c = parsebuf[parseindex++];
		if (c != '\0')
			return (c);
		free(parsebuf);
		parsebuf = NULL;
		parseindex = 0;
		goto restart;
	}

	c = getc(fin);
	if (c == '\\') {
		next = getc(fin);
		if (next != '\n') {
			ungetc(next, fin);
			return (c);
		}
		yylval.lineno = lineno;
		lineno++;
		goto restart;
	}
	if (c == '#') {
		c = getc(fin);
		while (c != '\n' && c != EOF)
			c = getc(fin);
	}
	if (c == '\n') {
		yylval.lineno = lineno;
		lineno++;
	}
	return (c);
}

int
lungetc(int c, FILE *fin)
{
	if (parsebuf && parseindex) {
		/* XXX breaks on index 0 */
		parseindex--;
		return (c);
	}
	if (c == '\n') {
		yylval.lineno = lineno;
		lineno--;
	}
	return ungetc(c, fin);
}

int
findeol()
{
	int c;

	if (parsebuf) {
		free(parsebuf);
		parsebuf = NULL;
		parseindex = 0;
	}

	/* skip to either EOF or the first real EOL */
	while (1) {
		c = lgetc(fin);
		if (c == '\n')
			break;
		if (c == EOF)
			break;
	}
	return (ERROR);
}

int
yylex(void)
{
	char buf[8096], *p;
	int endc, c;
	int token;

	p = buf;
	while ((c = lgetc(fin)) == ' ' || c == '\t' || c == '\n')
		;

	yylval.lineno = lineno;
	if (c == '#')
		while ((c = lgetc(fin)) != '\n' && c != EOF)
			;

	switch (c) {
	case '\'':
	case '"':
		endc = c;
		while (1) {
			if ((c = lgetc(fin)) == EOF)
				return (0);
			if (c == endc) {
				*p = '\0';
				break;
			}
			if (c == '\n')
				continue;
			if (p + 1 >= buf + sizeof(buf) - 1) {
				yyerror("string too long");
				return (findeol());
			}
			*p++ = (char)c;
		}
		yylval.v.string = strdup(buf);
		if (yylval.v.string == NULL)
			err(1, "yylex: strdup");
		return (STRING);
	}

	if (isdigit(c)) {
		int index = 0, base = 10;
		u_int64_t n = 0;

		yylval.v.number = 0;
		while (1) {
			if (base == 10) {
				if (!isdigit(c))
					break;
				c -= '0';
			} else if (base == 16) {
				if (isdigit(c))
					c -= '0';
				else if (c >= 'a' && c <= 'f')
					c -= 'a' - 10;
				else if (c >= 'A' && c <= 'F')
					c -= 'A' - 10;
				else
					break;
			}
			n = n * base + c;

			c = lgetc(fin);
			if (c == EOF)
				break;
			if (index++ == 0 && n == 0 && c == 'x') {
				base = 16;
				c = lgetc(fin);
				if (c == EOF)
					break;
			}
		}
		yylval.v.number = (u_int32_t)n;

		if (c != EOF)
			lungetc(c, fin);
		if (debug > 1)
			fprintf(stderr, "number: %d\n", yylval.v.number);
		return (NUMBER);
	}

#define allowed_in_string(x) \
	(isalnum(x) || (ispunct(x) && x != '(' && x != ')' && \
	x != '{' && x != '}' && x != '<' && x != '>' && \
	x != '!' && x != '=' && x != '/' && x != '#' && \
	x != ',' && x != '(' && x != ')'))

	if (isalnum(c)) {
		do {
			*p++ = c;
			if (p-buf >= sizeof buf) {
				yyerror("string too long");
				return (ERROR);
			}
		} while ((c = lgetc(fin)) != EOF && (allowed_in_string(c)));
		lungetc(c, fin);
		*p = '\0';
		token = lookup(buf);
		yylval.v.string = strdup(buf);
		if (yylval.v.string == NULL)
			err(1, "yylex: strdup");
		return (token);
	}
	if (c == EOF)
		return (0);
	return (c);
}

int
parse_config(const char *n, struct matrix **m)
{
	infile = n;
	fin = fopen(infile, "rb");
	if (fin == NULL) {
		perror("fopen() failed");
		return (1);
	}
	matrices = m;
	lineno = 1;
	errors = 0;
	yyparse();
	fclose(fin);
	return (errors ? -1 : 0);
}
