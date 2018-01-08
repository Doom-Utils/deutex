AC_INIT([DeuTex], [5.1.1], [https://github.com/Doom-Utils/deutex/issues])
AC_PACKAGE_URL([https://github.com/Doom-Utils/deutex])
AM_INIT_AUTOMAKE([foreign])
AC_PROG_CC
AC_PROG_CC_C99

AM_SILENT_RULES([yes])

AC_CHECK_PROG([ASCIIDOC], [a2x], [yes])
AM_CONDITIONAL([ASCIIDOC], [test "$ASCIIDOC" = yes])

AC_ARG_WITH([libpng],
    AS_HELP_STRING([--without-libpng],
                   [Build without libpng @<:@default=check@:>@]),
    [],
    [[with_libpng=check]])
AS_IF([test "$with_libpng" != no],
    [PKG_CHECK_MODULES([PNG], [libpng >= 1.6.0],
        [AC_DEFINE([HAVE_LIBPNG], [1], [libpng installed])],
        [AS_IF([test "$with_libpng" != check],
        [AC_MSG_FAILURE([--with-libpng was given, but test for libpng failed])
]) ]) ])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
    Makefile
    man/Makefile
    man/deutex.txt
    pkg.w32/config.make
    src/Makefile
])
AC_OUTPUT
