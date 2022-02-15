# Autoconf requirements
# Force autoconf to be at least this version number:
AC_PREREQ([2.69])
# Initialize, but use more options.  Note parameters brackets and whitespace.
AC_INIT([shalib], [2.1], [w7og@yahoo.com])
# Safety check - list a source file that wouldn't be in other directories:
AC_CONFIG_SRCDIR([src/sha2.c])

# Put autotools auxiliary files in a subdir, so they don't clutter top dir:
AC_CONFIG_AUX_DIR([build-aux])

AC_CONFIG_MACRO_DIR([m4])

AC_CANONICAL_TARGET
#AM_GNU_GETTEXT([external])
#AM_GNU_GETTEXT_VERSION(0.17)
AX_GENERATE_CHANGELOG

# Enable "automake" to simplify creating makefiles:
AM_INIT_AUTOMAKE([1.11 subdir-objects -Wall -Werror])

# Look for a C compiler:
AC_PROG_CC([gcc cl cc])
#AC_PROG_CXX
AM_PROG_AS
AC_PROG_INSTALL
AC_PROG_CPP
#AC_PROG_AWK
#AC_PROG_LN_S
#AC_PROG_MAKE_SET
AC_PROG_MKDIR_P
AM_PROG_CC_C_O
AC_C_BIGENDIAN

sha2_version_1=`echo $PACKAGE_VERSION | cut -d . -f 1`
sha2_version_2=`echo $PACKAGE_VERSION | cut -d . -f 2`
sha2_version_3=`echo $PACKAGE_VERSION | cut -d . -f 3`
sha2_version=$sha2_version_1.$sha2_version_2.$sha2_version_3
AC_DEFINE_UNQUOTED(SHA2_VERSION,  "$sha2_version",  [ SHA2 version string.  ])
AC_DEFINE_UNQUOTED(SHA2_VERSION_1, $sha2_version_1, [ SHA2 version level 1. ])
AC_DEFINE_UNQUOTED(SHA2_VERSION_2, $sha2_version_2, [ SHA2 version level 2. ])
AC_DEFINE_UNQUOTED(SHA2_VERSION_3, $sha2_version_3, [ SHA2 version level 3. ])
AC_SUBST(SHA2_VERSION, $sha2_version)
# Perhaps check for: (more) programs, libraries, header files, types,
# structures, compiler characteristics, library functions, system services.

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lgmp':
AC_CHECK_LIB([gmp], [main])
# FIXME: Replace `main' with a function in `-lz':
AC_CHECK_LIB([z], [main])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h float.h limits.h malloc.h stddef.h stdint.h time.h ctype.h stdlib.h stdio.h intrin.h string.h sys/ioctl.h sys/time.h unistd.h getopt.h])
#AC_CHECK_HEADERS([zlib.h gmp.h])
# Checks for typedefs, structures, macros, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for compiler characteristics.
AC_C_CONST
AC_C_INLINE

AC_DEFINE(_GNU_SOURCE, 1, [ Define to enable GNU extensions in glibc ])
AC_HEADER_STDC

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_ALLOCA
AC_CHECK_FUNCS([bzero clock_gettime fchdir fdatasync floor ftruncate gettimeofday memmove memset mkdir munmap pow realpath rmdir sqrt strchr strdup strerror strtol isascii isblank])
AC_CHECK_FUNCS([getopt_long],,
  [ # FreeBSD has a "gnugetopt" library.
    AC_CHECK_LIB([gnugetopt], [getopt_long],
                 [ AC_DEFINE([HAVE_GETOPT_LONG]) ]) ])


# Options
AC_ARG_ENABLE(profile,
  AC_HELP_STRING([--enable-profile],
                 [enable profiling with gprof @<:@default=disabled@:>@]),
  [ sha2_profile="$enableval" ],
  [ sha2_profile="no" ])

AC_ARG_ENABLE(debug,
  AC_HELP_STRING(
    [--enable-debug],
    [enable development-time debugging @<:@default=disabled@:>@]),
  [ sha2_debug="$enableval" ],
  [ sha2_debug="no" ])
if test "$sha2_debug" = "yes"; then
  AM_CONDITIONAL(DEBUG, true)
  AC_DEFINE(DEBUG, 1,
    [ Define if you want SHA2 to print debug messages to stdout. ])
else
  AM_CONDITIONAL(DEBUG, false)
  AC_DEFINE(NDEBUG, 1, [ Define if you want to disable debug assertions. ])
fi

if test "$sha2_profile" = "yes"; then
  CFLAGS="$CFLAGS -pg"
  AM_CONDITIONAL(SHA2_PROFILE, true)
else
  AM_CONDITIONAL(SHA2_PROFILE, false)
fi

AC_ARG_ENABLE(warnings,
  AC_HELP_STRING(
    [--disable-warnings],
    [disable C compiler warning messages @<:@default=enabled@:>@]),
  [ sha2_cc_warnings="$enableval" ],
  [ sha2_cc_warnings="yes" ])
if test "$sha2_cc_warnings" = "yes"; then
  VL_PROG_CC_WARNINGS()
fi

AC_ARG_WITH(alloca,
  AC_HELP_STRING(
    [--without-alloca],
    [dont use alloca  @<:@default=use@:>@]),
  [ sha2_use_alloca="$withval" ],
  [ sha2_use_alloca="yes" ])
if test "$sha2_use_alloca" = "yes"; then
  ALLOCA=""
  AC_FUNC_ALLOCA
  if test -z "$ALLOCA"; then
    # alloca() works.
    AC_DEFINE(SHA2_USE_ALLOCA, 1,
      [ Define if you want sha2 to use alloca() instead of malloc() when
        allocating memory needed for test operations. ])
  fi
fi

# check the shalib2 compile defines

AC_ARG_ENABLE(test,
  AC_HELP_STRING(
    [--enable-test],
    [enable validation testing @<:@default=disabled@:>@]),
  [ sha2_test="$enableval" ],
  [ sha2_test="no" ])
if test "$sha2_test" = "yes"; then
  AM_CONDITIONAL(TEST_VECTORS, true)
  AC_DEFINE(TEST_VECTORS, 1,
    [ Select if you want to verify correct hash calculations. ])
else
  AM_CONDITIONAL(TEST_VECTORS, false)
fi

AC_ARG_ENABLE(dotiming,
  AC_HELP_STRING(
    [--disable-dotiming],
    [disable timing loops @<:@default=enabled@:>@]),
  [ sha2_dotiming="$enableval" ],
  [ sha2_dotiming="yes" ])
if test "$sha2_dotiming" = "yes"; then
  AM_CONDITIONAL(DOTIMING, true)
  AC_DEFINE(DOTIMING, 1,
    [ Define if you want sha2 to print hash rate summaries to stdout. ])
else
  AM_CONDITIONAL(DOTIMING, false)
fi

AC_ARG_ENABLE(sha512only,
  AC_HELP_STRING(
    [--disable-sha512only],
    [enable sha512only option @<:@default=enabled@:>@]),
  [ sha2_sha512only="$enableval" ],
  [ sha2_sha512only="yes" ])
if test "$sha2_sha512only" = "yes"; then
  AM_CONDITIONAL(SHA512ONLY, true)
  AC_DEFINE(SHA512ONLY, 1,
    [ Define if you want sha2/shalib2 to only provide support for sha512. ])
else
  AM_CONDITIONAL(SHA512ONLY, false)
fi

AC_ARG_ENABLE(unrollloops,
  AC_HELP_STRING(
    [--disable-unrollloops],
    [disable the unrolling of loops @<:@default=enabled@:>@]),
  [ sha2_unrollloops="$enableval" ],
  [ sha2_unrollloops="yes" ])
if test "$sha2_unrollloops" = "yes"; then
  AM_CONDITIONAL(UNROLLLOOPS, true)
  AC_DEFINE(UNROLLLOOPS, 1,
    [ Define if you want to un-roll loops. ])
else
  AM_CONDITIONAL(UNROLLLOOPS, false)
fi

AC_ARG_ENABLE(ssesupport,
  AC_HELP_STRING(
    [--disable-ssesupport],
    [disable SSE/AVX support @<:@default=enabled@:>@]),
  [ sha2_ssesupport="$enableval" ],
  [ sha2_ssesupport="yes" ])
if test "$sha2_ssesupport" = "yes"; then
  AM_CONDITIONAL(SSESUPPORT, true)
  AC_DEFINE(SSESUPPORT, 1,
    [ Define if you want to include assembly coded SSE/AVX routines (64 bit OS only). ])
else
  AM_CONDITIONAL(SSESUPPORT, false)
fi


AC_SYS_LARGEFILE

#AC_LIBTOOL_TAGS([])
#AC_LIBTOOL_WIN32_DLL
AM_PROG_AR
LT_INIT([disable-shared])
AC_SUBST([LIBTOOL_DEPS])
#AC_PROG_LIBTOOL


# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([memchr memset strchr strerror strtol memcpy])

# Output files
# Put configuration results here, so we can easily #include them:
AC_CONFIG_HEADERS([src/config.h])
# other files to creat
AC_CONFIG_FILES([
Makefile
m4/Makefile
build-aux/Makefile
src/Makefile
])

# Do final output.
AC_OUTPUT


# Print configuration summary

cat <<EOF


Configuration summary
=====================

Sha2 is now configured as follows:

* Compilation environment

  CC       = $CC
  CFLAGS   = $CFLAGS
  CPP      = $CPP
  CPPFLAGS = $CPPFLAGS
  LD       = $LD
  LDFLAGS  = $LDFLAGS
  LIBS     = $LIBS
  Use alloca():				$sha2_use_alloca

* Sha2 options

  Development-time debugging:		$sha2_debug
  Print hash rate summaries:		$sha2_dotiming
  Only provide support for sha512:	$sha2_sha512only
  Perform loop un-rolling:		$sha2_unrollloops
  Include 64 bit SSE/AVX code:		$sha2_ssesupport
  Test the hash functions:		$sha2_test
  (Note: library not created if test is slected)


EOF

