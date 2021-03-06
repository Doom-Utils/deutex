AC_INIT([DeuTex], [5.2.2], [https://github.com/Doom-Utils/deutex/issues])
AC_PACKAGE_URL([https://github.com/Doom-Utils/deutex])
AM_INIT_AUTOMAKE([foreign])
AC_PROG_CC

AM_SILENT_RULES([yes])

AC_CHECK_PROG([ASCIIDOC], [a2x], [yes])
AC_ARG_ENABLE([man],
    AS_HELP_STRING([--disable-man],
                   [Disable manpage generation]),
    [], [enable_man=check]
)
AS_IF([test "x$enable_man" = "xyes"],
    AS_IF([test "x$ASCIIDOC" != "xyes"],
        AC_MSG_ERROR([AsciiDoc a2x program is required in order to build the manual])
    )
)
AM_CONDITIONAL([ASCIIDOC], [test "x$enable_man" != "xno" && test "x$ASCIIDOC" = "xyes"])

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
