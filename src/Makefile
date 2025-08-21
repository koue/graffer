#
LOCALBASE?=	/usr/local

PROG=		graffer

SRCS=		graffer.c data.c graph.c parse.y

.PATH:		${.CURDIR}/../contrib/gd
SRCS+=		gd.c gd_io.c gdfonts.c gdhelpers.c gd_security.c \
		gd_png.c gd_io_file.c

MAN=		graffer.8

CFLAGS+=	-Wall \
		-I${.CURDIR} \
		-I${.CURDIR}/../contrib/gd \
		-I${LOCALBASE}/include

LDFLAGS+=	-L${LOCALBASE}/lib

LDADD=		-lm -lpng

.include <bsd.prog.mk>
