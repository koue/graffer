/*
 * Copyright (c) 2020-2025 Nikola Kolev <koue@chaosophia.net>
 * Copyright (c) 2002-2006 Daniel Hartmeier
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
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <gd.h>
#include <gdfonts.h>

#include "pool.h"
#include "graph.h"

extern int	 debug;

double	 intersect(double, double, double, double);
void	 find_max(struct matrix *);
void	 normalize(struct matrix *);
void	 draw(gdImagePtr, struct matrix *, struct graph *, int, unsigned);
void	 draw_grid(gdImagePtr, struct matrix *);

int
graph_add_matrix(struct pool *pool, struct matrix **matrices,
    const char *filename, unsigned theme, unsigned width, unsigned height,
    unsigned beg, unsigned end)
{
	struct matrix *m;
	const unsigned fw = gdFontSmall->w, fh = gdFontSmall->h;

	m = pool_alloc(pool, sizeof(struct matrix));
	if (m == NULL)
		err(1, "pool_alloc");
	memset(m, 0, sizeof(struct matrix));
	m->filename = pool_strdup(pool, filename);
	if (m->filename == NULL)
		err(1, "pool_strdup");
	m->beg = beg;
	m->end = end;
	m->theme = theme;
	m->width = width;
	m->height = height;
	m->w0 = width-fh-18*fw-2*fw;
	m->h0 = height-5*fh;
	m->x0 = fh+9*fw;
	m->y0 = fh;
	m->graphs[0] = m->graphs[1] = NULL;
	if (*matrices == NULL)
		m->next = NULL;
	else
		m->next = *matrices;
	*matrices = m;
	return (0);
}

int
graph_add_graph(struct pool *pool, struct graph **graphs, unsigned width,
    unsigned desc_nr, const char *label, const char *unit, unsigned color,
    int filled, int bytes, int type)
{
	unsigned i;
	struct graph *g;

	g = pool_alloc(pool, sizeof(struct graph));
	if (g == NULL)
		err(1, "pool_alloc");
	memset(g, 0, sizeof(struct graph));
	g->unit = pool_strdup(pool, unit);
	if (g->unit == NULL)
		err(1, "pool_strdup");
	g->desc_nr = desc_nr;
	g->label = pool_strdup(pool, label);
	if (g->label == NULL)
		err(1, "pool_strdup");
	g->color = color;
	g->filled = filled;
	g->bytes = bytes;
	g->type = type;
	g->data = pool_alloc(pool, width * sizeof(double));
	if (g->data == NULL)
		err(1, "graph_add_graph: pool_alloc");
	for (i = 0; i < width; ++i)
		g->data[i] = 0.0;
	g->data_max = 0.0;
	if (*graphs == NULL)
		g->next = NULL;
	else
		g->next = *graphs;
	*graphs = g;
	return (0);
}

int
graph_generate_images(struct matrix *matrices)
{
	struct matrix *matrix;
	const unsigned fw = gdFontSmall->w, fh = gdFontSmall->h;

	matrix = matrices;
	while (matrix != NULL) {
		gdImagePtr im;
		int backcolor;
		FILE *out;
		struct graph *graph;
		unsigned i, x;

		if (debug > 0)
			printf("graph_generate_images: generating file %s\n",
			    matrix->filename);

		find_max(matrix);
		for (i = 0; i < 2; ++i) {
			double m = 0.0;

			for (graph = matrix->graphs[i]; graph;
			    graph = graph->next)
				if (graph->data_max > m)
					m = graph->data_max;
			for (graph = matrix->graphs[i]; graph;
			    graph = graph->next)
				graph->data_max = m;
			if (debug)
				printf("graph_generate_images: maximum %s "
				    "%.2f\n", i ? "right" : "left", m);
		}
		normalize(matrix);

		im = gdImageCreate(matrix->width, matrix->height);
		if (matrix->theme == 0)
			backcolor = gdImageColorAllocate(im, 255, 255, 255);
		else
			backcolor = gdImageColorAllocate(im, 0, 0, 0);

		gdImageFilledRectangle(im, 0, 0, matrix->width-1,
		    matrix->height-1, backcolor);

		draw_grid(im, matrix);

		for (i = 0; i < 2; ++i) {
			x = i == 0 ? matrix->x0 : matrix->x0+matrix->w0;
			for (graph = matrix->graphs[i]; graph;
			    graph = graph->next) {
				int color = gdImageColorAllocate(im,
				    (graph->color >> 16) & 0xFF,
				    (graph->color >> 8) & 0xFF,
				    graph->color & 0xFF);

				draw(im, matrix, graph,
				    color, graph->filled);
				if (i)
					x -= strlen(graph->label) * fw;
				gdImageString(im, gdFontSmall, x,
				    matrix->y0+matrix->h0+5+fh,
				    graph->label, color);
				if (!i)
					x += (strlen(graph->label) + 1) * fw;
				else
					x -= fw;
			}
		}

		out = fopen(matrix->filename, "wb");
		if (out == NULL) {
			warn("%s", matrix->filename);
			return (1);
		}
		gdImagePng(im, out);
		fclose(out);
		gdImageDestroy(im);

		matrix = matrix->next;
	}

	return (0);
}

void
find_max(struct matrix *m)
{
	unsigned i, j;
	struct graph *g;

	for (i = 0; i < 2; ++i)
		for (g = m->graphs[i]; g != NULL; g = g->next) {
			g->data_max = 0.0;
			for (j = 0; j < m->w0; ++j)
				if (g->data[j] > g->data_max)
					g->data_max = g->data[j];
		}
}

void
normalize(struct matrix *m)
{
	unsigned i, j;
	struct graph *g;

	for (i = 0; i < 2; ++i)
		for (g = m->graphs[i]; g != NULL; g = g->next)
			if (g->data_max != 0.0)
				for (j = 0; j < m->w0; ++j)
					g->data[j] /= g->data_max;
}

void
draw(gdImagePtr im, struct matrix *m, struct graph *g, int color,
    unsigned filled)
{
	unsigned x = m->x0, y = m->y0, w = m->w0, h = m->h0;
	unsigned dx, dy0 = 0;
	int border = gdImageColorAllocate(im, 0, 64, 96);

	for (dx = 0; dx < w; ++dx) {
		unsigned dy = g->data[dx] * h;

		if (filled) {
			gdImageLine(im, x+dx+1, y+h-1, x+dx+1, y+h-1-dy,
			    color);
			if (dx > 0)
				gdImageLine(im, x+dx, y+h-1-dy0, x+dx+1,
				    y+h-1-dy, border);
		}
		else if (dx > 0)
			gdImageLine(im, x+dx, y+h-1-dy0, x+dx+1, y+h-1-dy,
			    color);
		dy0 = dy;
	}
}

void
scale_unit(double *m, char *k, int bytes)
{
	*k = ' ';
	if (bytes)
		*m *= 8.0;
	if (*m >= 1000) {
		*m /= bytes ? 1024 : 1000;
		*k = 'k';
	}
	if (*m >= 1000) {
		*m /= bytes ? 1024 : 1000;
		*k = 'm';
	}
	if (*m >= 1000) {
		*m /= bytes ? 1024 : 1000;
		*k = 'g';
	}
	if (*m >= 1000) {
		*m /= bytes ? 1024 : 1000;
		*k = 't';
	}
}

void
draw_grid(gdImagePtr im, struct matrix *matrix)
{
	const unsigned fw = gdFontSmall->w, fh = gdFontSmall->h;
	unsigned x0 = matrix->x0, y0 = matrix->y0,
	    w0 = matrix->w0, h0 = matrix->h0;
	unsigned len = matrix->end - matrix->beg;
	double max[2] = { 0.0, 0.0 };
	const char *unit[2] = { NULL, NULL };
	unsigned i, ii;
	char k[2];
	double dx;
	char *t;
	int frontcolor;
	int grey = gdImageColorAllocate(im, 220, 220, 220);

	if (matrix->theme == 0)
		frontcolor = gdImageColorAllocate(im,   0,   0,   0);
	else
		frontcolor = gdImageColorAllocate(im, 255, 255, 255);
	if (matrix->graphs[0] != NULL) {
		max[0] = matrix->graphs[0]->data_max;
		scale_unit(&max[0], &k[0], matrix->graphs[0]->bytes);
		unit[0] = matrix->graphs[0]->unit;
	}
	if (matrix->graphs[1] != NULL) {
		max[1] = matrix->graphs[1]->data_max;
		scale_unit(&max[1], &k[1], matrix->graphs[1]->bytes);
		unit[1] = matrix->graphs[1]->unit;
	}

	/* bounding box */
	gdImageLine(im, x0, y0+h0, x0+w0, y0+h0, frontcolor);
	gdImageLine(im, x0, y0, x0, y0+h0, frontcolor);
	gdImageLine(im, x0+w0, y0, x0+w0, y0+h0, frontcolor);

	/* horizontal units */
	ii = h0 / fh;
	if (ii > 10)
		ii = 10;
	for (i = 0; i <= ii; ++i) {
		char t[10];
		unsigned y1 = y0+(i*h0)/ii;

		gdImageLine(im, x0-2, y1, x0, y1, frontcolor);
		gdImageLine(im, x0+w0, y1, x0+w0+2, y1, frontcolor);
		if (i < ii)
			gdImageLine(im, x0+1, y1, x0+w0-1, y1, grey);
		if (matrix->graphs[0] != NULL) {
			snprintf(t, sizeof(t), "%5.1f %c",
			    (max[0]*(ii-i))/ii, k[0]);
			gdImageString(im, gdFontSmall, fh+fw,
			    y1-fh/2, t, frontcolor);
		}
		if (matrix->graphs[1] != NULL) {
			snprintf(t, sizeof(t), "%5.1f %c",
			    (max[1]*(ii-i))/ii, k[1]);
			gdImageString(im, gdFontSmall, x0+w0+1*fw,
			    y1-fh/2, t, frontcolor);
		}
	}

	/* time label */
	t = "minutes";
	dx = (60.0 * (double)w0) / (double)len; // pixels per unit
	if (dx < 4*fw) {
		dx *= 60.0; t = "hours";
		if (dx < 4*fw) {
			dx *= 24.0; t = "days";
			if (dx < 4*fw) {
				dx *= 7.0; t = "weeks";
				if (dx < 4*fw) {
					dx *= 30.0 / 7.0; t = "months";
					if (dx < 4*fw) {
						dx *= 365.0 / 30.0;
						t = "years";
					}
				}
			}
		}
	}
	/* time units */
	for (i = 0; (i*dx) <= w0; ++i) {
		unsigned x1 = x0+w0-(i*dx);
		char t[64];

		gdImageLine(im, x1, y0+h0, x1, y0+h0+2, frontcolor);
		if (x1 > x0 && x1 < x0+w0)
			gdImageLine(im, x1, y0+h0, x1, y0, grey);
		if (x1 < x0+w0) {
			snprintf(t, sizeof(t), "-%u", i);
			gdImageString(im, gdFontSmall, x1-(strlen(t)*fw)/2,
			    y0+h0+5, t, frontcolor);
		} else {
			time_t tt = time(0);
			strncpy(t, asctime(localtime(&tt)), 24); t[24] = 0;
			gdImageString(im, gdFontSmall, x0+(w0-strlen(t)*fw)/2,
			    y0+h0+5+2*fh, t, frontcolor);
		}
	}
	/* hours/days/weeks/months */
	gdImageString(im, gdFontSmall, x0+w0-strlen(t)*fw, y0+h0+5, t, frontcolor);
	if (matrix->graphs[0] != NULL)
		gdImageStringUp(im, gdFontSmall, 0,
		    y0+h0-(h0-strlen(unit[0])*fw)/2, (char *)unit[0], frontcolor);
	if (matrix->graphs[1] != NULL)
		gdImageStringUp(im, gdFontSmall, x0+w0+8*fw,
		    y0+h0-(h0-strlen(unit[1])*fw)/2, (char *)unit[1], frontcolor);
}

