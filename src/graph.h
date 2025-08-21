/*
 * Copyright (c) 2025 Nikola Kolev <koue@chaosophia.net>
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

#ifndef _GRAPH_H_
#define _GRAPH_H_

struct graph {
	unsigned	 desc_nr;
	char		*label;
	char		*unit;
	u_int32_t	 color;	/* 0x00RRGGBB */
	int		 filled;
	int		 bytes;
	int		 type;
	unsigned	 t[2];
	u_int64_t	 v[2];
	double		*data;
	double		 data_max;
	struct graph	*next;
};

struct matrix {
	char		*filename;
	unsigned	 beg, end;
	unsigned	 theme;
	unsigned	 width, height;
	unsigned	 w0, h0, x0, y0;
	struct graph	*graphs[2];
	struct matrix	*next;
};

int	 graph_add_matrix(struct matrix **, const char *, unsigned, unsigned,
	    unsigned, unsigned, unsigned);
int	 graph_add_graph(struct graph **, unsigned, unsigned, const char *,
	    const char *, u_int32_t, int, int, int);
int	 graph_generate_images(struct matrix *);

#endif
