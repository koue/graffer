# 

PROG=	graffer
SRCS=	graffer.c data.c graph.c parse.y
MAN=	graffer.8
CFLAGS+=	-Wall
CFLAGS+=	-I/usr/local/include -I${.CURDIR}
LDFLAGS+=	-L/usr/local/lib -L${X11BASE}/lib
LDFLAGS+=	-lgd -lm -lpng -lz -ljpeg

.include <bsd.prog.mk>
