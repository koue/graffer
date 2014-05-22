# 

PROG=	graffer
SRCS=	gd.c gd_io.c gdfonts.c gdfx.c gdhelpers.c gd_security.c gdtables.c gdft.c gd_png.c gd_io_dp.c gd_io_file.c gd_jpeg.c graffer.c data.c graph.c parse.y
MAN=	graffer.8
CFLAGS+=	-Wall
CFLAGS+=	-I/usr/local/include -I${.CURDIR}
LDFLAGS+=	-L/usr/local/lib -L${X11BASE}/lib
LDFLAGS+=	-lm -lpng -lz -ljpeg

.include <bsd.prog.mk>
