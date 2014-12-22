# Makefile.am -- Process this file with automake to produce Makefile.in

bin_PROGRAMS = b2fxec gpd unfxe zda_compressor zda_tester

b2fxec_SOURCES	=	src/b2fxec/cb.cpp src/b2fxec/chuff.cpp \
			src/b2fxec/compress_fxe0.cpp src/b2fxec/compress_fxe2.cpp \
			src/b2fxec/compress_fxe3.cpp src/b2fxec/fio.cpp \
			src/b2fxec/headers.cpp src/b2fxec/lz2min.cpp \
			src/b2fxec/main.cpp src/b2fxec/port.cpp \
			src/b2fxec/unfxe.cpp src/b2fxec/b2fxec_io.h \
			src/b2fxec/bitutils.h src/b2fxec/cb.h src/b2fxec/chuff.h \
			src/b2fxec/compress.h src/b2fxec/compress_fxe0.h \
			src/b2fxec/compress_fxe2.h src/b2fxec/compress_fxe3.h \
			src/b2fxec/dol_def.h src/b2fxec/fio.h src/b2fxec/fxe_def.h \
			src/b2fxec/headers.h src/b2fxec/logo.h src/b2fxec/lz2min.h \
			src/b2fxec/params.h src/b2fxec/port.h src/b2fxec/unfxe.h

gpd_SOURCES	=	src/gpd/arm-dis.c src/gpd/arm-dis.h src/gpd/bitutils.h \
			src/gpd/disasm.c src/gpd/disasm.h src/gpd/dis-asm.h \
			src/gpd/fxe.c src/gpd/fxe.h src/gpd/main.c

unfxe_SOURCES	=	src/unfxe/unfxe.c src/unfxe/bitutils.h


zda_compressor_SOURCES	=	src/zda_compressor/zda_compressor.c

zda_compressor_LDADD	=	$(ZLIB_LIBS)

zda_tester_SOURCES	=	src/zda_compressor/zda_tester.c

zda_tester_LDADD	=	$(ZLIB_LIBS)

EXTRA_DIST = autogen.sh
