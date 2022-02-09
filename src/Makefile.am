.asm.o:
	yasm -f elf64 -o $@ $<

if TEST_VECTORS
bin_PROGRAMS = $(top_builddir)/bin/sha2 

__top_builddir__bin_sha2_SOURCES = sha2.c \
	linuxpw2hash.c \
	myCpuInfo.c \
	../include/sha2.h
	
if SSESUPPORT
__top_builddir__bin_sha2_SOURCES += sha512_avx2_rorx.asm  \
	sha512_avx.asm  \
	sha512_sse4.asm
endif
else
noinst_LIBRARIES = $(top_builddir)/lib/libsha2.a
#shalib2_a_LDFLAGS = -no-undefined -version-info 5:0:0 $(LDFLAGS)
__top_builddir__lib_libsha2_a_LIBADD = $(LIBINTL)

noinst_HEADERS = ../include/sha2.h

__top_builddir__lib_libsha2_a_SOURCES = sha2.c \
	linuxpw2hash.c \
	myCpuInfo.c
	
if SSESUPPORT
__top_builddir__lib_libsha2_a_SOURCES += sha512_avx2_rorx.asm  \
	sha512_avx.asm  \
	sha512_sse4.asm
endif

endif


# __top_builddir__bin_classifier_SOURCES = Classifier.h \
# 	Classifier.cpp \
# 	SequenceInfo.h \
# 	SequenceInfo.cpp \
# 	KmerMatcher.h \
# 	KmerMatcher.cpp \
# 	KmerMatcherOptions.h \
# 	KmerMatcherOptions.cpp \
# 	Timer.h \
# 	Timer.cpp \
# 	types.hpp \
# 	custom_utils.hpp \
# 	data_structures.hpp


AM_CFLAGS = -I $(srcdir)/../include -W -Wall -pedantic -pthread
AM_LDFLAGS = -static

