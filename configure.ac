AC_INIT([DeuTex], [4.5.0-alpha], [https://github.com/Doom-Utils/deutex/issues])
AC_PACKAGE_URL([https://github.com/Doom-Utils/deutex])
AM_INIT_AUTOMAKE([foreign])
AC_PROG_CC
AC_PROG_CC_C99

AM_SILENT_RULES([yes])

AC_CHECK_PROG([ASCIIDOC], [a2x], [yes])
AM_CONDITIONAL([ASCIIDOC], [test x$ASCIIDOC = xyes])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
    Makefile
    man/Makefile
    man/deutex.txt
    src/Makefile
])
AC_OUTPUT
